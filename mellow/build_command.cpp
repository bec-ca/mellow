#include "build_command.hpp"

#include "build_engine.hpp"
#include "defaults.hpp"

#include "command/command_builder.hpp"
#include "command/file_path.hpp"

namespace fs = std::filesystem;

using bee::FilePath;
using bee::ok;
using bee::OrError;
using std::is_same_v;
using std::optional;
using std::string;

namespace mellow {

namespace {

struct RunBuildArgs {
  optional<string> profile_name;
  bool verbose;
  bool force_build;
  FilePath output_dir;
  bool update_test_output;
  string mbuild_name;
  string build_config;
};

OrError<> run_build(const RunBuildArgs& args)
{
  auto output_dir =
    FilePath::of_std_path(fs::absolute(args.output_dir.to_std_path()));
  bail_unit(BuildEngine::build({
    .root_source_dir = FilePath::of_string("./"),
    .mbuild_name = args.mbuild_name,
    .build_config = args.build_config,
    .profile_name = args.profile_name,
    .output_dir_base = output_dir,
    .external_packages_dir = Defaults::external_packages_dir(output_dir),
    .verbose = args.verbose,
    .force_build = args.force_build,
    .update_test_output = args.update_test_output,
  }));

  P("Done");
  return ok();
}

} // namespace

command::Cmd BuildCommand::command()
{
  using namespace command::flags;
  auto builder = command::CommandBuilder("Build all");
  auto profile = builder.optional("--profile", string_flag);
  auto verbose = builder.no_arg("--verbose");
  auto force_build = builder.no_arg("--force-build");
  auto update_test_output = builder.no_arg("--update-test-output");
  auto output_dir = builder.optional_with_default(
    "--output-dir", file_path, Defaults::output_dir());
  auto mbuild_name = builder.optional_with_default(
    "--mbuild-name", string_flag, Defaults::mbuild_name);
  auto build_config = builder.optional_with_default(
    "--build-config", string_flag, ".build-config");
  return builder.run([=]() {
    return run_build({
      .profile_name = *profile,
      .verbose = *verbose,
      .force_build = *force_build,
      .output_dir = *output_dir,
      .update_test_output = *update_test_output,
      .mbuild_name = *mbuild_name,
      .build_config = *build_config,
    });
  });
}

} // namespace mellow
