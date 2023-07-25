#include "build_engine.hpp"

#include <chrono>
#include <filesystem>
#include <future>
#include <map>
#include <optional>
#include <ratio>
#include <set>
#include <string>
#include <vector>

#include "build_config.hpp"
#include "build_normalizer.hpp"
#include "build_task.hpp"
#include "generate_build_config.hpp"
#include "generated_mbuild_parser.hpp"
#include "package_path.hpp"
#include "task_manager.hpp"

#include "bee/error.hpp"
#include "bee/file_reader.hpp"
#include "bee/file_writer.hpp"
#include "bee/filesystem.hpp"
#include "bee/format_filesystem.hpp"
#include "bee/format_set.hpp"
#include "bee/format_vector.hpp"
#include "bee/os.hpp"
#include "bee/string_util.hpp"
#include "bee/sub_process.hpp"
#include "bee/util.hpp"
#include "diffo/diff.hpp"
#include "yasf/cof.hpp"

using bee::always_false_v;
using bee::compose_set;
using bee::compose_vector;
using bee::concat;
using bee::concat_many;
using bee::FilePath;
using bee::FileReader;
using bee::FileSystem;
using bee::FileWriter;
using bee::insert;
using bee::Span;
using bee::SubProcess;
using std::map;
using std::nullopt;
using std::optional;
using std::promise;
using std::set;
using std::shared_ptr;
using std::string;
using std::thread;
using std::vector;

namespace fs = std::filesystem;

namespace mellow {
namespace {

struct CommandRunner {
  struct Args {
    const FilePath& output_prefix;
    const FilePath cmd;
    const vector<string>& args = {};
    const optional<FilePath>& cwd = nullopt;
    const Span timeout;
    const bool verbose;
  };
  const FilePath cmd;
  const vector<string> args;
  const optional<FilePath> cwd;

  const FilePath stdout_path;
  const FilePath stderr_path;

  const Span timeout;
  const bool verbose;

  CommandRunner(const Args& args)
      : cmd(args.cmd),
        args(args.args),
        cwd(args.cwd),
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
          "Command '$ $' failed, error:'$', stderr:\n$\nstdout:\n$",
          cmd,
          args,
          err,
          stderr_content,
          stdout_content);
      }
    };

    bail_unit(FileSystem::mkdirs(stdout_path.parent()));
    bail_unit(FileSystem::mkdirs(stderr_path.parent()));
    if (verbose) { P("Running $ $...", cmd, args); }
    auto ret = SubProcess::spawn({
      .cmd = cmd,
      .args = args,
      .stdout_spec = stdout_path,
      .stderr_spec = stderr_path,
      .cwd = cwd,
    });

    if (ret.is_error()) { return tag_error(ret.error()); }
    auto& pid = ret.value();

    promise<bee::OrError<>> wait_result_promise;
    auto wait_result = wait_result_promise.get_future();

    thread waiter_thread(
      [pid, wait_result_promise = std::move(wait_result_promise)]() mutable {
        try {
          auto ret = pid->wait();
          wait_result_promise.set_value(std::move(ret));
        } catch (const std::exception& exn) {
          wait_result_promise.set_value(exn);
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

struct RunTest {
  const CommandRunner run_command;

  struct Args {
    const PackagePath& rule_name;
    const FilePath& root_build_dir;
    const FilePath& test_binary;
    const FilePath& expected;
    bool update_test_output;
  };

  const FilePath expected;
  const FilePath diff_output;
  const FilePath ok_path;
  const bool update_test_output;

  RunTest(const Args& args)
      : run_command({
          .output_prefix = args.rule_name.to_filesystem(args.root_build_dir),
          .cmd = args.test_binary,
          .timeout = Span::of_minutes(1),
        }),
        expected(args.expected),
        diff_output(args.rule_name.append_no_sep(".diff").to_filesystem(
          args.root_build_dir)),
        ok_path(args.rule_name.append_no_sep(".ok").to_filesystem(
          args.root_build_dir)),
        update_test_output(args.update_test_output)
  {}

  bee::OrError<> operator()() const
  {
    auto result = run_command();
    if (result.is_error()) { return result.error(); }
    const auto& stdout_path = run_command.stdout_path;

    if (update_test_output) { return copy_if_differs(stdout_path, expected); }

    bail(
      diff,
      diffo::Diff::diff_files(
        expected.to_std_path(), stdout_path.to_std_path()));
    if (diff.empty()) { return FileSystem::touch_file(ok_path); }

    vector<string> msg;

    for (const auto& diff_line : diff) {
      if (diff_line.action == diffo::Action::Equal) { continue; }
      msg.push_back(
        F("$:$: $ $",
          expected,
          diff_line.line_number,
          diffo::Diff::action_prefix(diff_line.action),
          diff_line.line));
    }
    return bee::Error::fmt("Test failed:\n$", bee::join(msg, "\n"));
  }
};

struct RunGenRule {
  const FilePath binary;
  const vector<string> flags;
  const FilePath root_build_dir;
  const NormalizedRule::ptr nrule;
  const vector<string> outputs;

  struct output_info {
    FilePath path;
    FilePath run_dir_path;
  };

  bee::OrError<> operator()() const
  {
    FilePath run_dir = nrule->package_name.to_filesystem(root_build_dir);

    const CommandRunner run_command({
      .output_prefix = run_dir,
      .cmd = binary,
      .args = flags,
      .cwd = run_dir,
      .timeout = Span::of_minutes(1),
    });

    vector<output_info> output_info;
    for (const auto& output_name : outputs) {
      auto output = nrule->package_name / output_name;
      output_info.push_back(
        {.path = nrule->package_dir / output_name,
         .run_dir_path = output.to_filesystem(root_build_dir)});
    }
    for (const auto& output : output_info) {
      FileSystem::remove(output.run_dir_path);
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

struct RunSystemLib {
  const FilePath root_build_dir;
  const gmp::SystemLib rrule;
  const PackagePath system_lib_config;

  bee::OrError<> operator()() const
  {
    auto run = [&](const string& arg) -> bee::OrError<vector<string>> {
      auto system_lib_config = SubProcess::OutputToString::create();
      auto stderr_spec = SubProcess::OutputToString::create();
      auto args = compose_vector(rrule.flags, arg);
      auto ret = SubProcess::run({
        .cmd = rrule.command,
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

    auto content = yasf::Cof::serialize(SystemLibConfig{
      .cpp_flags = cflags,
      .ld_libs = libs,
    });

    auto output_path = system_lib_config.to_filesystem(root_build_dir);

    bail_unit(FileSystem::mkdirs(output_path.parent()));
    bail_unit(FileWriter::save_file(output_path, content));

    return bee::ok();
  }
};

struct RunCppRule {
  using ptr = shared_ptr<RunCppRule>;

  struct Args {
    const FilePath root_build_dir;
    const gmp::Profile profile;
    const bool is_library = false;
    const NormalizedRule::ptr nrule;
    const bc::Cpp build_config;
    const bool verbose;
  };

  bee::OrError<> operator()() const
  {
    set<FilePath> lib_dep_objects;

    if (!_main_output.has_value()) { return bee::ok(); }
    auto& main_output = *_main_output;

    bail_unit(FileSystem::mkdirs(main_output.parent()));

    auto fp_set_to_string_vec = [](const std::set<FilePath>& s) {
      return bee::map_vector(
        bee::to_vector(s), [](const auto& p) { return p.to_string(); });
    };

    vector<string> cmd_args = compose_vector(
      _cpp_flags,
      fp_set_to_string_vec(_input_sources),
      fp_set_to_string_vec(_input_objects),
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
      .cmd = FilePath::of_string(_compiler),
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
        insert(system_lib_configs, cfg->to_filesystem(args.root_build_dir));
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
      include_dirs.insert(lib->root_source_dir);
    }
    include_dirs.insert(nrule.root_source_dir);

    optional<FilePath> main_output;
    {
      optional<PackagePath> pmain_output;
      if (args.is_library) {
        if (!input_sources.empty()) {
          pmain_output = nrule.output_cpp_object();
        } else {
          pmain_output = nullopt;
        }
      } else {
        pmain_output = nrule.name;
      }
      if (pmain_output.has_value()) {
        main_output = pmain_output->to_filesystem(args.root_build_dir);
      }
    }

    vector<string> cpp_flags = compose_vector(
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

    set<FilePath> inputs = compose_set(
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
    vector<string> parts = compose_vector(_cpp_flags, _compiler);
    return bee::join(parts, "##");
  }

  RunCppRule(
    const PackagePath& name,
    optional<FilePath>&& main_output,
    string&& compiler,
    vector<string>&& cpp_flags,
    bool is_library,
    set<FilePath>&& input_objects,
    set<FilePath>&& input_sources,
    set<FilePath>&& system_lib_configs,
    set<FilePath>&& inputs,
    set<FilePath>&& outputs,
    const bool verbose)
      : _name(name),
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
  const string _compiler;
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
    const NormalizedRule::ptr& nrule, bool is_library = false)
  {
    auto runner = RunCppRule::create({
      .root_build_dir = _root_build_dir,
      .profile = *_profile,
      .is_library = is_library,
      .nrule = nrule,
      .build_config = _build_config.cpp_config(),
      .verbose = _verbose,
    });

    auto wrapper = [runner]() { return (*runner)(); };

    _manager->create_task({
      .key = nrule->name.append_no_sep(".compile"),
      .root_build_dir = _root_build_dir,
      .run = wrapper,
      .inputs = runner->inputs(),
      .outputs = runner->outputs(),
      .non_file_inputs_key = runner->non_file_inputs_key(),
    });

    return runner;
  }

  bee::OrError<> handle_rule(const gmp::Profile&, const NormalizedRule::ptr&)
  {
    return bee::ok();
  }

  bee::OrError<> handle_rule(
    const gmp::ExternalPackage&, const NormalizedRule::ptr&)
  {
    return bee::ok();
  }

  bee::OrError<> handle_rule(
    const gmp::CppBinary&, const NormalizedRule::ptr& nrule)
  {
    bail(cpp_rule, handle_cpp_rule(nrule));
    auto ret = _binaries.emplace(cpp_rule->name(), cpp_rule);
    if (!ret.second) {
      return bee::Error::fmt("Binary $ declared twice", cpp_rule->name());
    }
    return bee::ok();
  }

  bee::OrError<> handle_rule(
    const gmp::CppLibrary&, const NormalizedRule::ptr& nrule)
  {
    bail(cpp_rule, handle_cpp_rule(nrule, true));
    auto ret = _library_rules.emplace(cpp_rule->name(), cpp_rule);
    if (!ret.second) {
      return bee::Error::fmt("Library $ declared twice", cpp_rule->name());
    }
    return bee::ok();
  }

  bee::OrError<> handle_rule(
    const gmp::CppTest& rrule, const NormalizedRule::ptr& nrule)
  {
    bool should_run = true;
    auto os_filter = nrule->os_filter();
    if (!os_filter.empty()) {
      should_run = false;
      for (const auto& os : os_filter) {
        if (os == "linux") {
          if (bee::RunningOS == bee::OS::Linux) { should_run = true; }
        } else if (os == "macos") {
          if (bee::RunningOS == bee::OS::Macos) { should_run = true; }
        } else {
          return bee::Error::fmt("Invalid os name in os_filter: $", os);
        }
      }
    }

    if (!should_run) { return bee::ok(); }

    bail(binary_rule, handle_cpp_rule(nrule));

    auto rule_name = nrule->name;
    auto test_output = nrule->package_dir / rrule.output;

    assert(
      binary_rule->main_output().has_value() && "A binary must have an output");
    auto binary_file = *binary_rule->main_output();

    auto runner = RunTest({
      .rule_name = rule_name,
      .root_build_dir = _root_build_dir,
      .test_binary = binary_file,
      .expected = test_output,
      .update_test_output = _update_test_output,
    });

    _manager->create_task({
      .key = rule_name.append_no_sep(".run"),
      .root_build_dir = _root_build_dir,
      .run = std::move(runner),
      .inputs = {binary_file, test_output},
      .outputs = {runner.ok_path},
    });

    return bee::ok();
  }

  bee::OrError<> handle_rule(
    const gmp::GenRule& rrule, const NormalizedRule::ptr& nrule)
  {
    auto name = nrule->name;
    const auto& pkg = nrule->package_name;

    PackagePath binary_rule_name = pkg / rrule.binary;

    auto it = _binaries.find(binary_rule_name);
    if (it == _binaries.end()) {
      return bee::Error::fmt(
        "Gen rule $ depends on unknown binary $", name, binary_rule_name);
    }

    auto binary_rule = it->second;

    set<FilePath> outputs;
    for (const auto& output : rrule.outputs) {
      outputs.insert(nrule->package_dir / output);
    }

    auto binary_path = binary_rule->main_output();
    if (!binary_path.has_value()) {
      return bee::Error::fmt(
        "Gen rul $ depends on binary $, but rule does not produce a binary",
        name,
        binary_rule_name);
    }

    _manager->create_task({
      .key = name.append_no_sep(".run"),
      .root_build_dir = _root_build_dir,
      .run = RunGenRule({
        .binary = *binary_path,
        .flags = rrule.flags,
        .root_build_dir = _root_build_dir,
        .nrule = nrule,
        .outputs = rrule.outputs,
      }),
      .inputs = {*binary_path},
      .outputs = outputs,
    });

    return bee::ok();
  }

  bee::OrError<> handle_rule(
    const gmp::SystemLib& rrule, const NormalizedRule::ptr& nrule)
  {
    auto system_lib_config_opt = nrule->system_lib_config();
    assert(
      system_lib_config_opt.has_value() &&
      "system_lib rule must have a system_lib_config");
    auto system_lib_config = *system_lib_config_opt;

    set<FilePath> outputs;
    outputs.insert(system_lib_config.to_filesystem(_root_build_dir));

    _manager->create_task({
      .key = nrule->name.append_no_sep(".run"),
      .root_build_dir = _root_build_dir,
      .run = RunSystemLib({
        .root_build_dir = _root_build_dir,
        .rrule = rrule,
        .system_lib_config = system_lib_config,
      }),
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

  bee::OrError<> select_profile(const vector<gmp::Profile>& profiles)
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

  bee::OrError<> run() { return _manager->run(_force_build); }

  static bee::OrError<Builder> create(const BuildEngine::Args& args)
  {
    if (!fs::exists(args.build_config)) {
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
        _profile_name(args.profile_name),
        _update_test_output(args.update_test_output),
        _verbose(args.verbose),
        _force_build(args.force_build),
        _manager(TaskManager::create())
  {}

  const BuildConfig _build_config;
  const FilePath _output_dir_base;
  const optional<string> _profile_name;
  const FilePath _root_source_dir;
  const bool _update_test_output;
  const bool _verbose;
  const bool _force_build;

  TaskManager::ptr _manager;
  map<PackagePath, RunCppRule::ptr> _library_rules;
  map<PackagePath, RunCppRule::ptr> _binaries;

  optional<gmp::Profile> _profile;
  FilePath _root_build_dir;
};

} // namespace

bee::OrError<> BuildEngine::build(const Args& args)
{
  BuildNormalizer norm(args.mbuild_name, args.external_packages_dir);
  bail(build, norm.normalize_build(args.root_source_dir));

  bail(builder, Builder::create(args));
  bail_unit(builder.select_profile(build.profiles));

  bail_unit(builder.prepare_rules(build.normalized_rules));

  bail_unit(builder.run());

  return {};
}

} // namespace mellow
