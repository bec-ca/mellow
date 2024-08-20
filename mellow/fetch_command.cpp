#include "fetch_command.hpp"

#include <filesystem>
#include <queue>

#include "defaults.hpp"
#include "mbuild_parser.hpp"

#include "bee/file_path.hpp"
#include "bee/filesystem.hpp"
#include "bee/format.hpp"
#include "bee/format_vector.hpp"
#include "bee/print.hpp"
#include "bee/sub_process.hpp"
#include "command/command_builder.hpp"
#include "command/file_path.hpp"

namespace fs = std::filesystem;

using bee::Error;
using bee::FilePath;
using bee::ok;
using bee::OrError;
using std::is_same_v;
using std::optional;
using std::string;

namespace mellow {

namespace {

OrError<> fetch_external_packges(const FilePath& output_dir)
{
  auto target_dir = Defaults::external_packages_dir(output_dir);
  auto tmp_dir = output_dir / ".downloads";
  bail_unit(bee::FileSystem::mkdirs(tmp_dir));

  std::set<string> seen_packages;

  std::queue<FilePath> dirs;

  auto fetch_pkgs_for =
    [&target_dir, &tmp_dir, &seen_packages](const FilePath& dir) -> OrError<> {
    bail(rules, MbuildParser::from_file(dir / Defaults::mbuild_name));
    for (auto& rule : rules) {
      auto pkg_opt =
        rule.visit([&]<class T>(T& rule) -> optional<types::ExternalPackage> {
          if constexpr (is_same_v<T, types::ExternalPackage>) {
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
      P("Fetching $...", pkg.name);
      bail_unit(bee::FileSystem::mkdirs(target_dir));
      auto dest = target_dir / pkg.name;

      FilePath source_dir;
      if (pkg.source.has_value()) {
        source_dir = FilePath(*pkg.source);
      } else if (pkg.url.has_value()) {
        auto pkg_tmp_dir = tmp_dir / pkg.name;
        bail_unit(bee::FileSystem::remove_all(pkg_tmp_dir));
        bail_unit(bee::FileSystem::mkdirs(pkg_tmp_dir));
        auto download_file = (pkg_tmp_dir / (pkg.name + ".tar.gz")).to_string();
        bail_unit(bee::SubProcess::run(
          {.cmd = FilePath("curl"),
           .args = {
             *pkg.url,
             "--location",
             "--silent",
             "--show-error",
             "--output",
             download_file}}));
        bail_unit(bee::SubProcess::run(
          {.cmd = FilePath("tar"),
           .args = {"-C", pkg_tmp_dir.to_string(), "-xzf", download_file}}));
        bail(content, bee::FileSystem::list_dir(pkg_tmp_dir));
        if (content.directories.size() != 1) {
          return Error::fmt(
            "$: External package download produced more than one "
            "directory: $",
            loc,
            content.directories);
        }
        source_dir = pkg_tmp_dir / content.directories[0];
      } else {
        return Error::fmt(
          "$: External package rule doesn't have a source or a url", loc);
      }

      bail_unit(bee::FileSystem::remove_all(dest));
      std::error_code ec;
      fs::copy(
        (source_dir / pkg.name).to_std_path(),
        dest.to_std_path(),
        fs::copy_options::recursive,
        ec);
      if (ec) {
        return Error::fmt(
          "$: Failed to process external package: $", loc, ec.message());
      }
    }
    return ok();
  };

  dirs.push(FilePath("."));

  while (!dirs.empty()) {
    auto dir = dirs.front();
    dirs.pop();
    bail_unit(fetch_pkgs_for(dir));
  }

  return ok();
}

} // namespace

using command::Cmd;
using command::CommandBuilder;

Cmd FetchCommand::command()
{
  namespace f = command::flags;
  auto builder = CommandBuilder("Fetch external pacakges");
  auto output_dir = builder.optional_with_default(
    "--output-dir", f::FilePath, Defaults::output_dir());
  return builder.run([=]() { return fetch_external_packges(*output_dir); });
}

} // namespace mellow
