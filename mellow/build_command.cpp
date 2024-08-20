#include "build_command.hpp"

#include "build_engine.hpp"
#include "defaults.hpp"
#include "repo.hpp"

#include "bee/filesystem.hpp"
#include "bee/print.hpp"
#include "command/command_builder.hpp"
#include "command/file_path.hpp"

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
  bool force_test;
  FilePath output_dir;
  bool update_test_output;
  string mbuild_name;
  FilePath build_config;
};

bee::OrError<bee::FilePath> canonical_path(
  const bee::FilePath& path, bool create = false)
{
  bail(abs, bee::FileSystem::absolute(path));
  if (create) { bail_unit(bee::FileSystem::mkdirs(abs)); }
  return bee::FileSystem::canonical(abs);
}

OrError<> run_build(const RunBuildArgs& args)
{
  bail(output_dir, canonical_path(args.output_dir, true));
  bail(cwd, bee::FileSystem::current_dir());
  bail(cwd_can, canonical_path(cwd));
  bail(repo_root_dir, Repo::root_dir(cwd_can));
  bail_unit(BuildEngine::build({
    .repo_root_dir = repo_root_dir,
    .mbuild_name = args.mbuild_name,
    .build_config = args.build_config,
    .profile_name = args.profile_name,
    .output_dir_base = output_dir,
    .external_packages_dir = Defaults::external_packages_dir(output_dir),
    .verbose = args.verbose,
    .force_build = args.force_build,
    .force_test = args.force_test,
    .update_test_output = args.update_test_output,
  }));

  P("Done");
  return ok();
}

} // namespace

command::Cmd BuildCommand::command()
{
  namespace f = command::flags;
  auto builder = command::CommandBuilder("Build all");
  auto profile = builder.optional("--profile", f::String);
  auto verbose = builder.no_arg("--verbose");
  auto force_build = builder.no_arg("--force-build");
  auto force_test = builder.no_arg("--force-test");
  auto update_test_output = builder.no_arg("--update-test-output");
  auto output_dir = builder.optional_with_default(
    "--output-dir", f::FilePath, Defaults::output_dir());
  auto mbuild_name = builder.optional_with_default(
    "--mbuild-name", f::String, Defaults::mbuild_name);
  auto build_config = builder.optional("--build-config", f::FilePath);
  return builder.run([=]() {
    auto build_config_path =
      build_config->value_or(*output_dir / ".build-config");
    return run_build({
      .profile_name = *profile,
      .verbose = *verbose,
      .force_build = *force_build,
      .force_test = *force_test,
      .output_dir = *output_dir,
      .update_test_output = *update_test_output,
      .mbuild_name = *mbuild_name,
      .build_config = build_config_path,
    });
  });
}

} // namespace mellow
