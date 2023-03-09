#include "build_engine.hpp"
#include "config_command.hpp"
#include "genbuild.hpp"
#include "mbuild_parser.hpp"

#include "bee/file_reader.hpp"
#include "bee/file_writer.hpp"
#include "bee/filesystem.hpp"
#include "bee/format.hpp"
#include "bee/sort.hpp"
#include "bee/sub_process.hpp"
#include "bee/util.hpp"
#include "command/command_builder.hpp"
#include "command/group_builder.hpp"
#include "mellow/generated_mbuild_parser.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <queue>
#include <ranges>
#include <system_error>

namespace fs = std::filesystem;

using bee::always_false_v;
using bee::Error;
using bee::FilePath;
using bee::ok;
using bee::OrError;
using bee::print_line;
using bee::Unit;
using std::is_same_v;
using std::optional;
using std::string;

namespace mellow {
namespace {

FilePath default_mbuild_name = FilePath::of_string("mbuild");
FilePath default_output_dir = FilePath::of_string("build");

FilePath default_external_packages_dir(const bee::FilePath& output_dir)
{
  return output_dir / "external-packages";
}

OrError<Unit> run_print()
{
  auto config_or_error = MbuildParser::from_file(default_mbuild_name);
  if (config_or_error.is_error()) {
    return Error::format("Failed to parse config:\n$", config_or_error.error());
  }
  auto config = std::move(config_or_error.value());
  auto serialized = MbuildParser::to_string(config);

  print_line(serialized);

  return ok();
}

struct RunBuildArgs {
  optional<string> profile_name;
  bool verbose;
  bool force_build;
  string output_dir;
  bool update_test_output;
  string mbuild_file;
  string build_config;
};

OrError<Unit> run_build(const RunBuildArgs& args)
{
  auto output_dir = FilePath::of_std_path(fs::absolute(args.output_dir));
  bail_unit(BuildEngine::build({
    .root_source_dir = FilePath::of_string("./"),
    .mbuild_name = args.mbuild_file,
    .build_config = args.build_config,
    .profile_name = args.profile_name,
    .output_dir_base = output_dir,
    .external_packages_dir = default_external_packages_dir(output_dir),
    .verbose = args.verbose,
    .force_build = args.force_build,
    .update_test_output = args.update_test_output,
  }));

  print_line("Done");
  return ok();
}

command::Cmd build_command()
{
  using namespace command::flags;
  auto builder = command::CommandBuilder("Build all");
  auto profile = builder.optional("--profile", string_flag);
  auto verbose = builder.no_arg("--verbose");
  auto force_build = builder.no_arg("--force-build");
  auto update_test_output = builder.no_arg("--update-test-output");
  auto output_dir = builder.optional_with_default(
    "--output-dir", string_flag, default_output_dir.to_string());
  auto mbuild_file = builder.optional_with_default(
    "--mbuild-file", string_flag, default_mbuild_name.to_string());
  auto build_config = builder.optional_with_default(
    "--build-config", string_flag, ".build-config");
  return builder.run([=]() {
    return run_build({
      .profile_name = *profile,
      .verbose = *verbose,
      .force_build = *force_build,
      .output_dir = *output_dir,
      .update_test_output = *update_test_output,
      .mbuild_file = *mbuild_file,
      .build_config = *build_config,
    });
  });
}

OrError<Unit> run_format(
  bool inplace, optional<string> filename_opt, bool check_only)
{
  if (inplace && check_only) {
    return Error(
      "The modes inplace and check-only can't be enabled at the same time");
  }
  bool has_filename = filename_opt.has_value() && filename_opt != "-";
  auto filename = FilePath::of_string(has_filename ? *filename_opt : "");

  if (inplace && !has_filename) {
    return Error("Filename required for inplace mode");
  }

  bail(
    unformatted_content,
    has_filename ? bee::FileReader::read_file(filename)
                 : bee::FileSystem::read_stream(std::cin));

  bail(rules, MbuildParser::from_string(filename, unformatted_content));

  for (auto& rule : rules) {
    visit(
      []<class T>(T& rule) {
        if constexpr (is_same_v<T, gmp::Profile>) {
        } else if constexpr (is_same_v<T, gmp::SystemLib>) {
        } else if constexpr (is_same_v<T, gmp::ExternalPackage>) {
        } else if constexpr (is_same_v<T, gmp::CppBinary>) {
          bee::sort(rule.sources);
          bee::sort(rule.libs);
        } else if constexpr (is_same_v<T, gmp::CppLibrary>) {
          bee::sort(rule.sources);
          bee::sort(rule.headers);
          bee::sort(rule.libs);
        } else if constexpr (is_same_v<T, gmp::CppTest>) {
          bee::sort(rule.sources);
          bee::sort(rule.libs);
        } else if constexpr (is_same_v<T, gmp::GenRule>) {
          bee::sort(rule.outputs);
        } else {
          static_assert(always_false_v<T> && "non exhaustive visit");
        }
      },
      rule.value);
  }

  auto formated_content = MbuildParser::to_string(rules);

  if (check_only) {
    if (formated_content != unformatted_content) {
      return Error("Format diff found");
    }
  } else if (inplace) {
    bail_unit(bee::FileWriter::save_file(filename, formated_content));
  } else {
    std::cout << formated_content;
  }

  return bee::unit;
}

OrError<Unit> fetch_external_packges(const bee::FilePath& output_dir)
{
  auto target_dir = default_external_packages_dir(output_dir);
  auto tmp_dir = output_dir / ".downloads";
  bail_unit(bee::FileSystem::mkdirs(tmp_dir));

  std::set<string> seen_packages;

  std::queue<FilePath> dirs;

  auto fetch_pkgs_for = [&target_dir, &tmp_dir, &seen_packages](
                          const FilePath& dir) -> OrError<Unit> {
    bail(rules, MbuildParser::from_file(dir / default_mbuild_name));
    for (auto& rule : rules) {
      auto pkg_opt =
        rule.visit([&]<class T>(T& rule) -> optional<gmp::ExternalPackage> {
          if constexpr (is_same_v<T, gmp::ExternalPackage>) {
            return rule;
          } else {
            return std::nullopt;
          }
        });
      if (!pkg_opt.has_value()) { continue; }
      const auto& pkg = *pkg_opt;

      if (seen_packages.contains(pkg.name)) { continue; }
      seen_packages.insert(pkg.name);

      auto loc = pkg.location.has_value() ? pkg.location->hum() : "";
      print_line("fetching $...", pkg.name);
      bail_unit(bee::FileSystem::mkdirs(target_dir));
      auto dest = target_dir / pkg.name;

      FilePath source_dir;
      if (pkg.source.has_value()) {
        source_dir = FilePath::of_string(*pkg.source);
      } else if (pkg.url.has_value()) {
        auto pkg_tmp_dir = tmp_dir / pkg.name;
        bail_unit(bee::FileSystem::mkdirs(pkg_tmp_dir));
        auto download_file = (pkg_tmp_dir / (pkg.name + ".tar.gz")).to_string();
        bail_unit(bee::SubProcess::run(
          {.cmd = "curl",
           .args = {*pkg.url, "--location", "-o", download_file}}));
        bail_unit(bee::SubProcess::run(
          {.cmd = "tar",
           .args = {"-C", pkg_tmp_dir.to_string(), "-xzf", download_file}}));
        bail(content, bee::FileSystem::list_dir(pkg_tmp_dir));
        if (content.directories.size() != 1) {
          return Error::format(
            "$: External package download produced more than one "
            "directory",
            loc);
        }
        source_dir = content.directories[0];
      } else {
        return Error::format(
          "$: External package rule doesn't have a source or a url", loc);
      }

      fs::remove_all(dest.to_std_path());
      std::error_code ec;
      fs::copy(
        (source_dir / pkg.name).to_std_path(),
        dest.to_std_path(),
        fs::copy_options::recursive,
        ec);
      if (ec) {
        return Error::format(
          "$: Failed to process external package: $", loc, ec.message());
      }
    }
    return ok();
  };

  dirs.push(FilePath::of_string("."));

  while (!dirs.empty()) {
    auto dir = dirs.front();
    dirs.pop();
    bail_unit(fetch_pkgs_for(dir));
  }

  return ok();
}

using command::Cmd;
using command::CommandBuilder;
using command::GroupBuilder;

Cmd format_command()
{
  using namespace command::flags;
  auto builder = CommandBuilder("Format mbuild file");
  auto inplace = builder.no_arg("--inplace");
  auto check_only = builder.no_arg("--check-only");
  auto filename = builder.anon(string_flag, "mbuild-file");
  return builder.run(
    [=]() { return run_format(*inplace, *filename, *check_only); });
}

Cmd genbuild_command()
{
  using namespace command::flags;
  auto builder = CommandBuilder("Generate an mbuild file");
  auto directory = builder.anon(string_flag, "dir");
  auto output = builder.optional("--output-mbuild", string_flag);
  return builder.run([=]() { return GenBuild::run(*directory, *output); });
}

Cmd fetch_command()
{
  using namespace command::flags;
  auto builder = CommandBuilder("Fetch external pacakges");
  auto output_dir = builder.optional_with_default(
    "--output-dir", string_flag, default_output_dir.to_string());
  return builder.run([=]() {
    return fetch_external_packges(bee::FilePath::of_string(*output_dir));
  });
}

Cmd print_command()
{
  using namespace command::flags;
  auto builder = CommandBuilder("Generate an mbuild file");
  return builder.run([=]() { return run_print(); });
}

} // namespace

int main(int argc, char* argv[])
{
  return GroupBuilder("Mellow")
    .cmd("print", print_command())
    .cmd("format", format_command())
    .cmd("build", build_command())
    .cmd("genbuild", genbuild_command())
    .cmd("fetch", fetch_command())
    .cmd("config", ConfigCommand::command())
    .build()
    .main(argc, argv);
}

} // namespace mellow

int main(int argc, char* argv[]) { return mellow::main(argc, argv); }
