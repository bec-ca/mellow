#include "build_engine.hpp"

#include <future>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "build_config.hpp"
#include "build_normalizer.hpp"
#include "generate_build_config.hpp"
#include "mbuild_types.generated.hpp"
#include "package_path.hpp"
#include "runable_rule.hpp"
#include "task_manager.hpp"

#include "bee/file_reader.hpp"
#include "bee/filesystem.hpp"
#include "bee/format_optional.hpp"
#include "bee/format_vector.hpp"
#include "bee/or_error.hpp"
#include "bee/os.hpp"
#include "bee/print.hpp"
#include "bee/string_util.hpp"
#include "bee/sub_process.hpp"
#include "bee/util.hpp"
#include "diffo/diff.hpp"
#include "yasf/cof.hpp"

using bee::compose_vector;
using bee::concat;
using bee::concat_many;
using bee::FilePath;
using bee::FileReader;
using bee::FileSystem;
using bee::Span;
using std::optional;
using std::set;
using std::string;
using std::vector;

namespace mellow {
namespace {

struct CommandRunner {
  struct Args {
    FilePath output_prefix;
    FilePath cmd;
    vector<string> args{};
    optional<FilePath> cwd{};
    set<FilePath> data{};
    Span timeout;
    bool verbose{false};
  };
  const FilePath cmd;
  const vector<string> args;
  const optional<FilePath> cwd;

  const set<FilePath> data;

  const FilePath stdout_path;
  const FilePath stderr_path;

  const Span timeout;
  const bool verbose;

  CommandRunner(Args&& args)
      : cmd(std::move(args.cmd)),
        args(std::move(args.args)),
        cwd(std::move(args.cwd)),
        data(std::move(args.data)),
        stdout_path(args.output_prefix + ".stdout"),
        stderr_path(args.output_prefix + ".stderr"),
        timeout(args.timeout),
        verbose(args.verbose)
  {}

  string non_file_inputs_key() const
  {
    vector<string> parts;
    concat(parts, cmd.to_string());
    concat(parts, args);
    return bee::join(parts, "##");
  }

  bee::OrError<> operator()() const
  {
    auto tag_error = [this](const bee::OrError<>& err) -> bee::OrError<> {
      if (!err.is_error()) {
        return bee::ok();
      } else {
        auto stderr_content = FileReader::read_file(stderr_path).value_or("");
        auto stdout_content = FileReader::read_file(stdout_path).value_or("");
        return bee::Error::fmt(
          "Command '$ $' cwd:$ failed, error:'$', stderr:\n$\nstdout:\n$",
          cmd,
          args,
          cwd,
          err,
          stderr_content,
          stdout_content);
      }
    };

    bail_unit(FileSystem::mkdirs(stdout_path.parent()));
    bail_unit(FileSystem::mkdirs(stderr_path.parent()));
    if (verbose) { P("Running $ $...", cmd, args); }

    if (cwd.has_value()) {
      bail_unit(FileSystem::mkdirs(*cwd));
      for (const auto& d : data) {
        auto link = *cwd / d.filename();
        if (bee::FileSystem::exists(link)) {
          bail_unit(bee::FileSystem::remove(link));
        }
        std::error_code err;
        bail_unit(bee::FileSystem::create_symlink(d, link));
      }
    }

    auto ret = bee::SubProcess::spawn({
      .cmd = cmd,
      .args = args,
      .stdout_spec = stdout_path,
      .stderr_spec = stderr_path,
      .cwd = cwd,
    });

    if (ret.is_error()) { return tag_error(ret.error()); }
    auto& pid = ret.value();

    std::promise<bee::OrError<>> wait_result_promise;
    auto wait_result = wait_result_promise.get_future();

    std::thread waiter_thread(
      [pid, wait_result_promise = std::move(wait_result_promise)]() mutable {
        try {
          auto ret = pid->wait();
          wait_result_promise.set_value(std::move(ret));
        } catch (const std::exception& exn) {
          wait_result_promise.set_value(bee::Error(exn));
        }
      });

    auto check_status = [&]() -> bee::OrError<> {
      auto status = wait_result.wait_for(timeout.to_chrono());
      if (
        status == std::future_status::timeout ||
        status == std::future_status::deferred) {
        bail_unit(pid->kill());
        return tag_error(bee::Error::fmt("Command timed out after $", timeout));
      } else {
        return tag_error(wait_result.get());
      }
    };

    {
      auto ret = check_status();
      waiter_thread.join();
      return ret;
    }
  }
};

bool equal_files(const FilePath& file1, const FilePath& file2)
{
  {
    auto size1 = FileSystem::file_size(file1);
    auto size2 = FileSystem::file_size(file2);
    if (size1.is_error() || size2.is_error() || *size1 != *size2) {
      return false;
    }
  }
  auto content1 = FileReader::read_file(file1);
  auto content2 = FileReader::read_file(file2);
  return !content1.is_error() && !content2.is_error() && *content1 == *content2;
}

bee::OrError<> copy_if_differs(const FilePath& from, const FilePath& to)
{
  if (equal_files(from, to)) { return bee::ok(); }
  return FileSystem::copy(from, to);
}

struct RunTest final : public RunableRule {
  const CommandRunner run_command;

  struct Args {
    PackagePath rule_name;
    FilePath root_build_dir;
    FilePath test_binary;
    FilePath expected;
    bool update_test_output;
  };

  const FilePath expected;
  const bool update_test_output;

  RunTest(Args&& args)
      : RunableRule(true),
        run_command({
          .output_prefix = args.rule_name.to_filesystem(args.root_build_dir),
          .cmd = args.test_binary,
          .timeout = Span::of_minutes(1),
        }),
        expected(args.expected),
        update_test_output(args.update_test_output)
  {}

  virtual bee::OrError<> run() const override
  {
    auto result = run_command();
    if (result.is_error()) { return result.error(); }
    const auto& stdout_path = run_command.stdout_path;

    if (update_test_output) { return copy_if_differs(stdout_path, expected); }

    bail(
      diff,
      diffo::Diff::diff_files(
        expected,
        stdout_path,
        {.treat_missing_files_as_empty = true, .context_lines = 0}));

    if (!diff.empty()) {
      vector<string> msg;
      for (const auto& chunk : diff) {
        for (const auto& diff_line : chunk.lines) {
          msg.push_back(
            F("$:$: $ $",
              expected,
              diff_line.line_number,
              diffo::Diff::action_prefix(diff_line.action),
              diff_line.line));
        }
      }
      return bee::Error::fmt("Test failed:\n$", bee::join(msg, "\n"));
    }

    return bee::ok();
  }
};

struct RunGenRule final : public RunableRule {
  struct Args {
    FilePath binary;
    vector<string> flags;
    FilePath root_build_dir;
    FilePath repo_root_dir;
    NormalizedRule::ptr nrule;
    vector<string> outputs;
  };

  RunGenRule(Args&& args) : RunableRule(false), _args(std::move(args)) {}

  struct output_info {
    FilePath path;
    FilePath run_dir_path;
  };

  virtual bee::OrError<> run() const override
  {
    FilePath run_dir =
      _args.nrule->package_name.to_filesystem(_args.root_build_dir);

    set<FilePath> abs_data;
    for (const auto& d : _args.nrule->data()) {
      abs_data.insert(_args.repo_root_dir / d);
    }

    const CommandRunner run_command({
      .output_prefix = run_dir,
      .cmd = _args.binary,
      .args = _args.flags,
      .cwd = run_dir,
      .data = abs_data,
      .timeout = Span::of_minutes(1),
    });

    vector<output_info> output_info;
    for (const auto& output_name : _args.outputs) {
      auto output = _args.nrule->package_name / output_name;
      output_info.push_back(
        {.path = _args.nrule->package_dir / output_name,
         .run_dir_path = output.to_filesystem(_args.root_build_dir)});
    }
    for (const auto& output : output_info) {
      if (FileSystem::exists(output.run_dir_path)) {
        bail_unit(FileSystem::remove(output.run_dir_path));
      }
    }
    bail_unit(run_command());
    for (const auto& output : output_info) {
      if (!FileSystem::exists(output.run_dir_path)) {
        return bee::Error::fmt("Expected output not generated: $", output.path);
      }
    }
    for (const auto& info : output_info) {
      bail_unit(copy_if_differs(info.run_dir_path, info.path));
    }
    return bee::ok();
  }

 private:
  const Args _args;
};

struct SystemLibConfig {
  vector<string> cpp_flags;
  vector<string> ld_libs;

  using fmt = std::pair<vector<string>, vector<string>>;

  yasf::Value::ptr to_yasf_value() const
  {
    return yasf::ser(fmt(cpp_flags, ld_libs));
  }

  static bee::OrError<SystemLibConfig> of_yasf_value(
    const yasf::Value::ptr& value)
  {
    bail(p, yasf::des<fmt>(value));
    return SystemLibConfig{
      .cpp_flags = p.first,
      .ld_libs = p.second,
    };
  }
};

struct RunSystemLib final : public RunableRule {
  struct Args {
    FilePath root_build_dir;
    types::SystemLib rrule;
    PackagePath system_lib_config;
  };

  RunSystemLib(Args&& args) : RunableRule(false), _args(std::move(args)) {}

  virtual bee::OrError<> run() const override
  {
    auto run = [&](const string& arg) -> bee::OrError<vector<string>> {
      auto system_lib_config = bee::SubProcess::OutputToString::create();
      auto stderr_spec = bee::SubProcess::OutputToString::create();
      auto args = compose_vector(_args.rrule.flags, arg);
      auto ret = bee::SubProcess::run({
        .cmd = _args.rrule.command,
        .args = args,
        .stdout_spec = system_lib_config,
        .stderr_spec = stderr_spec,
      });
      if (ret.is_error()) {
        bail(stderr_content, stderr_spec->get_output());
        return bee::Error::fmt("$:\nstderr:\n$", ret.error(), stderr_content);
      }
      bail(flags_str, system_lib_config->get_output());
      return bee::split_space(flags_str);
    };

    bail(libs, run("--libs"));
    bail(cflags, run("--cflags"));

    auto output_path =
      _args.system_lib_config.to_filesystem(_args.root_build_dir);

    bail_unit(FileSystem::mkdirs(output_path.parent()));

    auto content = SystemLibConfig{
      .cpp_flags = cflags,
      .ld_libs = libs,
    };
    return yasf::Cof::serialize_file(output_path, content);
  }

 private:
  const Args _args;
};

struct RunCppRule final : public RunableRule {
  using ptr = std::shared_ptr<RunCppRule>;

  struct Args {
    const FilePath root_build_dir;
    const types::Profile profile;
    const bool is_library = false;
    const NormalizedRule::ptr nrule;
    const generated::Cpp build_config;
    const bool verbose;
  };

  virtual bee::OrError<> run() const override
  {
    if (!_main_output.has_value()) { return bee::ok(); }
    auto& main_output = *_main_output;

    bail_unit(FileSystem::mkdirs(main_output.parent()));

    auto fp_set_to_string_set = [](const std::set<FilePath>& s) {
      return bee::map_set(s, [](auto&& p) { return p.to_string(); });
    };

    vector<string> cmd_args = compose_vector(
      _cpp_flags,
      fp_set_to_string_set(_input_sources),
      fp_set_to_string_set(_input_objects),
      "-o",
      main_output.to_string());

    for (const auto& system_lib_config : _system_lib_configs) {
      bail(content_str, FileReader::read_file(system_lib_config));
      bail(config, (yasf::Cof::deserialize<SystemLibConfig>(content_str)));
      if (_is_library) {
        concat(cmd_args, config.cpp_flags);
      } else {
        concat_many(cmd_args, config.ld_libs, config.cpp_flags);
      }
    }

    // if (!sources.empty()) {
    //   // We only care to produce the deps file if there are input sources
    //   auto dotd_file = name.append_no_sep(".d");
    //   auto dotd_fs = dotd_file.to_filesystem(_args.root_build_dir);
    //   concat_many(cmd_args, "-MMD", "-MF", dotd_fs);
    // }

    auto r = CommandRunner({
      .output_prefix = main_output,
      .cmd = FilePath(_compiler),
      .args = cmd_args,
      .timeout = Span::of_minutes(5),
      .verbose = _verbose,
    });
    return r();
  }

  static ptr create(const Args& args)
  {
    const auto& nrule = *args.nrule;

    auto input_sources = nrule.sources();

    set<FilePath> system_lib_configs;
    for (const auto& lib : nrule.transitive_libs) {
      if (auto cfg = lib->system_lib_config()) {
        bee::insert(
          system_lib_configs, cfg->to_filesystem(args.root_build_dir));
      }
    }

    set<FilePath> input_headers;
    {
      // we should use the headers from the .d file
      bee::insert(input_headers, nrule.headers());
      for (const auto& lib : nrule.transitive_libs) {
        bee::insert(input_headers, lib->headers());
      }
    }

    set<FilePath> input_objects;
    if (!args.is_library) {
      for (const auto& lib : nrule.transitive_libs) {
        if (auto obj = lib->output_cpp_object()) {
          input_objects.insert(obj->to_filesystem(args.root_build_dir));
        }
      }
    }

    set<FilePath> include_dirs;
    for (const auto& lib : nrule.transitive_libs) {
      include_dirs.insert(lib->root_package_dir);
    }
    include_dirs.insert(nrule.root_package_dir);

    optional<FilePath> main_output;
    {
      optional<PackagePath> pmain_output;
      if (args.is_library) {
        if (!input_sources.empty()) {
          pmain_output = nrule.output_cpp_object();
        } else {
          pmain_output = std::nullopt;
        }
      } else {
        pmain_output = nrule.name;
      }
      if (pmain_output.has_value()) {
        main_output = pmain_output->to_filesystem(args.root_build_dir);
      }
    }

    auto cpp_flags = compose_vector<string>(
      args.profile.cpp_flags, nrule.cpp_flags(), args.build_config.cpp_flags);
    for (const auto& dir : include_dirs) {
      concat_many(cpp_flags, "-iquote", dir.to_string());
    }
    for (const auto& lib : nrule.transitive_libs) {
      concat(cpp_flags, lib->cpp_flags());
    }
    if (args.is_library) {
      concat(cpp_flags, "-c");
    } else {
      concat_many(
        cpp_flags,
        args.profile.ld_flags,
        args.build_config.ld_flags,
        nrule.ld_flags());
    }

    auto inputs = bee::compose_set<FilePath>(
      input_sources, input_headers, input_objects, system_lib_configs);

    set<FilePath> outputs;
    if (main_output.has_value()) { outputs.insert(*main_output); }

    auto compiler =
      args.profile.cpp_compiler.value_or(args.build_config.compiler);

    return make_shared<RunCppRule>(
      nrule.name,
      std::move(main_output),
      std::move(compiler),
      std::move(cpp_flags),
      args.is_library,
      std::move(input_objects),
      std::move(input_sources),
      std::move(system_lib_configs),
      std::move(inputs),
      std::move(outputs),
      args.verbose);
  }

  const optional<FilePath>& main_output() const { return _main_output; }

  const set<FilePath> inputs() const { return _inputs; }
  const set<FilePath> outputs() const { return _outputs; }
  const PackagePath& name() const { return _name; }

  string non_file_inputs_key() const
  {
    vector<string> parts = compose_vector(_cpp_flags, _compiler.to_string());
    return bee::join(parts, "##");
  }

  RunCppRule(
    const PackagePath& name,
    optional<FilePath>&& main_output,
    FilePath&& compiler,
    vector<string>&& cpp_flags,
    bool is_library,
    set<FilePath>&& input_objects,
    set<FilePath>&& input_sources,
    set<FilePath>&& system_lib_configs,
    set<FilePath>&& inputs,
    set<FilePath>&& outputs,
    const bool verbose)
      : RunableRule(false),
        _name(name),
        _main_output(std::move(main_output)),
        _compiler(std::move(compiler)),
        _cpp_flags(std::move(cpp_flags)),
        _is_library(is_library),
        _input_objects(std::move(input_objects)),
        _input_sources(std::move(input_sources)),
        _system_lib_configs(std::move(system_lib_configs)),
        _inputs(std::move(inputs)),
        _outputs(std::move(outputs)),
        _verbose(verbose)
  {}

 private:
  const PackagePath _name;
  const optional<FilePath> _main_output;
  const FilePath _compiler;
  const vector<string> _cpp_flags;
  const bool _is_library;

  const set<FilePath> _input_objects;
  const set<FilePath> _input_sources;
  const set<FilePath> _system_lib_configs;
  const set<FilePath> _inputs;
  const set<FilePath> _outputs;

  const bool _verbose;
};

struct Builder {
  bee::OrError<RunCppRule::ptr> handle_cpp_rule(
    const NormalizedRule::ptr& nrule, bool is_library)
  {
    auto runner = RunCppRule::create({
      .root_build_dir = _root_build_dir,
      .profile = *_profile,
      .is_library = is_library,
      .nrule = nrule,
      .build_config = _build_config.cpp_config(),
      .verbose = _verbose,
    });

    _manager->create_task({
      .key = nrule->name.append_no_sep(".compile"),
      .root_build_dir = _root_build_dir,
      .run = runner,
      .inputs = runner->inputs(),
      .outputs = runner->outputs(),
      .non_file_inputs_key = runner->non_file_inputs_key(),
    });

    return runner;
  }

  bee::OrError<> handle_rule(const types::Profile&, const NormalizedRule::ptr&)
  {
    return bee::ok();
  }

  bee::OrError<> handle_rule(
    const types::ExternalPackage&, const NormalizedRule::ptr&)
  {
    return bee::ok();
  }

  bee::OrError<> handle_rule(
    const types::CppBinary&, const NormalizedRule::ptr& nrule)
  {
    bail(rule, handle_cpp_rule(nrule, false));
    _runable_rules.emplace(rule->name(), rule);
    return bee::ok();
  }

  bee::OrError<> handle_rule(
    const types::CppLibrary&, const NormalizedRule::ptr& nrule)
  {
    bail(rule, handle_cpp_rule(nrule, true));
    _runable_rules.emplace(rule->name(), rule);
    return bee::ok();
  }

  bee::OrError<> handle_rule(
    const types::CppTest& rrule, const NormalizedRule::ptr& nrule)
  {
    bool should_run = true;
    auto os_filter = nrule->os_filter();
    if (!os_filter.empty()) {
      should_run = false;
      for (const auto& os : os_filter) {
        switch (os) {
        case types::OS::linux:
          if (bee::RunningOS == bee::OS::Linux) { should_run = true; }
          break;
        case types::OS::macos:
          if (bee::RunningOS == bee::OS::Macos) { should_run = true; }
          break;
        }
      }
    }

    if (!should_run) { return bee::ok(); }

    bail(binary_rule, handle_cpp_rule(nrule, false));

    auto rule_name = nrule->name;
    auto test_output = nrule->package_dir / rrule.output;

    assert(
      binary_rule->main_output().has_value() && "A binary must have an output");
    auto binary_file = *binary_rule->main_output();

    auto runner = std::make_shared<RunTest>(RunTest::Args{
      .rule_name = rule_name,
      .root_build_dir = _root_build_dir,
      .test_binary = binary_file,
      .expected = test_output,
      .update_test_output = _update_test_output,
    });

    _manager->create_task({
      .key = rule_name.append_no_sep(".run"),
      .root_build_dir = _root_build_dir,
      .run = runner,
      .inputs = {binary_file, test_output},
      .outputs = {},
    });

    return bee::ok();
  }

  bee::OrError<bee::FilePath> find_binary_by_rule(const PackagePath& path)
  {
    auto it = _runable_rules.find(path);
    if (it == _runable_rules.end()) { return EF("Rule not found: $", path); }
    auto ptr = std::dynamic_pointer_cast<RunCppRule>(it->second);
    if (ptr == nullptr) {
      return EF("Rule $ found, but is not a cpp rule", path);
    }
    auto output = ptr->main_output();
    if (!output.has_value()) {
      return EF("Rule $ found, but has no output", path);
    }
    return std::move(*output);
  }

  bee::OrError<> handle_rule(
    const types::GenRule& rrule, const NormalizedRule::ptr& nrule)
  {
    auto name = nrule->name;
    const auto& pkg = nrule->package_name;

    PackagePath binary_rule_name = pkg / rrule.binary;
    bail(
      binary_path,
      find_binary_by_rule(binary_rule_name),
      "Failed to find binary for genrule '$'",
      name);

    set<FilePath> outputs;
    for (const auto& output : rrule.outputs) {
      outputs.insert(nrule->package_dir / output);
    }

    auto rule = std::make_shared<RunGenRule>(RunGenRule::Args{
      .binary = binary_path,
      .flags = rrule.flags,
      .root_build_dir = _root_build_dir,
      .repo_root_dir = _repo_root_dir,
      .nrule = nrule,
      .outputs = rrule.outputs,
    });
    _runable_rules.emplace(name, rule);

    set<FilePath> inputs;
    inputs.insert(binary_path);
    bee::insert(inputs, nrule->data());
    _manager->create_task({
      .key = name.append_no_sep(".run"),
      .root_build_dir = _root_build_dir,
      .run = rule,
      .inputs = inputs,
      .outputs = outputs,
    });

    return bee::ok();
  }

  bee::OrError<> handle_rule(
    const types::SystemLib& rrule, const NormalizedRule::ptr& nrule)
  {
    auto system_lib_config_opt = nrule->system_lib_config();
    assert(
      system_lib_config_opt.has_value() &&
      "system_lib rule must have a system_lib_config");
    auto system_lib_config = *system_lib_config_opt;

    set<FilePath> outputs;
    outputs.insert(system_lib_config.to_filesystem(_root_build_dir));

    auto rule = std::make_shared<RunSystemLib>(RunSystemLib::Args{
      .root_build_dir = _root_build_dir,
      .rrule = rrule,
      .system_lib_config = system_lib_config,
    });
    _runable_rules.emplace(nrule->name, rule);

    _manager->create_task({
      .key = nrule->name.append_no_sep(".run"),
      .root_build_dir = _root_build_dir,
      .run = rule,
      .inputs = {},
      .outputs = outputs,
    });

    return bee::ok();
  }

  bee::OrError<> prepare_rules(
    const vector<NormalizedRule::ptr>& normalized_rules)
  {
    for (const auto& nrule : normalized_rules) {
      auto result = nrule->raw_rule().visit(
        [&](const auto& rule) { return handle_rule(rule, nrule); });
      bail_unit(result);
    }

    return bee::ok();
  }

  bee::OrError<> select_profile(const vector<types::Profile>& profiles)
  {
    string profile_name = "default";
    vector<string> profile_flags;
    if (!profiles.empty()) {
      if (_profile_name.has_value()) {
        profile_name = _profile_name.value();
      } else {
        profile_name = profiles[0].name;
      }
      P("Using profile:$ ", profile_name);

      bool found = false;
      for (const auto& profile : profiles) {
        if (profile.name == profile_name) {
          _profile = profile;
          found = true;
          break;
        }
      }
      if (!found) {
        return bee::Error::fmt("Profile $ not found", profile_name);
      }
    }

    _root_build_dir = _output_dir_base / profile_name;
    bail_unit(FileSystem::mkdirs(_root_build_dir));

    return bee::ok();
  }

  bee::OrError<> run() { return _manager->run(); }

  static bee::OrError<Builder> create(const BuildEngine::Args& args)
  {
    if (!FileSystem::exists(args.build_config)) {
      PE(
        "Build config file '$' not found, creating one with default settings",
        args.build_config);
      bail_unit(GenerateBuildConfig::generate(args.build_config, {}));
    }

    bail(build_config, BuildConfig::load_from_file(args.build_config));
    return Builder(args, build_config);
  }

 private:
  Builder(const BuildEngine::Args& args, const BuildConfig& build_config)
      : _build_config(build_config),
        _output_dir_base(args.output_dir_base),
        _repo_root_dir(args.repo_root_dir),
        _profile_name(args.profile_name),
        _update_test_output(args.update_test_output),
        _verbose(args.verbose),
        _manager(TaskManager::create(
          {.force_build = args.force_build, .force_test = args.force_test}))
  {}

  const BuildConfig _build_config;
  const FilePath _output_dir_base;
  const FilePath _repo_root_dir;
  const optional<string> _profile_name;
  const bool _update_test_output;
  const bool _verbose;

  TaskManager::ptr _manager;
  std::map<PackagePath, RunableRule::ptr> _runable_rules;

  optional<types::Profile> _profile;
  FilePath _root_build_dir;
};

} // namespace

bee::OrError<> BuildEngine::build(const Args& args)
{
  BuildNormalizer norm(args.mbuild_name, args.external_packages_dir);
  bail(build, norm.normalize_build(args.repo_root_dir));

  bail(builder, Builder::create(args));
  bail_unit(builder.select_profile(build.profiles));

  bail_unit(builder.prepare_rules(build.normalized_rules));

  bail_unit(builder.run());

  return bee::ok();
}

} // namespace mellow
