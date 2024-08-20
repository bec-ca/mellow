#include "build_normalizer.hpp"

#include <map>
#include <string>

#include "mbuild_parser.hpp"
#include "mbuild_types.generated.hpp"
#include "normalized_rule.hpp"
#include "package_path.hpp"

#include "bee/file_path.hpp"
#include "bee/filesystem.hpp"
#include "bee/print.hpp"
#include "bee/string_util.hpp"
#include "bee/util.hpp"

using bee::Error;
using bee::FilePath;
using bee::OrError;
using std::is_same_v;
using std::map;
using std::optional;
using std::set;
using std::string;
using std::vector;

namespace mellow {
namespace {

const std::set<string> ignore_dirs = {
  "build",
  "build-ci",
  "publish",
};

OrError<vector<FilePath>> find_package_dirs(
  const FilePath& root_package_dir, const string& mbuild_name)
{
  vector<FilePath> output;
  bail(regular_files, bee::FileSystem::list_dir(root_package_dir));

  for (const auto& p : regular_files.directories) {
    if (ignore_dirs.contains(p) || p.starts_with(".")) { continue; }
    bail(subdirs, find_package_dirs(root_package_dir / p, mbuild_name));
    concat(output, subdirs);
  }
  for (const auto& p : regular_files.regular_files) {
    if (p == mbuild_name) { output.push_back(root_package_dir); }
  }

  return output;
}

template <class... Ts>
static void print_error_with_loc(
  const optional<yasf::Location>& loc, const char* fmt, Ts&&... args)
{
  auto msg = F(fmt, std::forward<Ts>(args)...);
  if (loc.has_value()) { msg = F("$: $", loc->hum(), msg); }
  P(msg);
}

OrError<vector<NormalizedRule::ptr>> top_sort(
  const map<PackagePath, NormalizedRule::ptr>& rules)
{
  set<NormalizedRule::ptr> done;
  vector<NormalizedRule::ptr> sorted_rules;

  while (true) {
    bool made_progress = false;
    bool all_done = true;
    for (const auto& [_, rule] : rules) {
      if (done.find(rule) != done.end()) { continue; }

      bool deps_done = true;
      set<NormalizedRule::ptr> transitive_libs;

      for (const auto& dep : rule->deps) {
        auto dep_rule = rules.find(dep);
        if (dep_rule == rules.end()) {
          print_error_with_loc(
            rule->location,
            "Rule '$' depends on unknown rule '$'",
            rule->name,
            dep);
          return Error("Invalid mbuild");
        }
        if (done.find(dep_rule->second) == done.end()) {
          deps_done = false;
          break;
        }
      }

      if (!deps_done) {
        all_done = false;
      } else {
        for (const auto& lib : rule->libs()) {
          auto lib_rule = rules.find(lib);
          if (lib_rule == rules.end()) {
            print_error_with_loc(
              rule->location,
              "Rule '$' depends on unknown lib '$'",
              rule->name,
              lib);
            return Error("Invalid mbuild");
          }
          transitive_libs.insert(lib_rule->second);
          bee::insert(transitive_libs, lib_rule->second->transitive_libs);
        }

        rule->transitive_libs = std::move(transitive_libs);
        sorted_rules.push_back(rule);
        done.insert(rule);
        made_progress = true;
      }
    }

    if (all_done) break;

    if (!made_progress) {
      // TODO: Identify dependency cycle
      std::vector<PackagePath> remaining;
      for (const auto& rule : sorted_rules) {
        if (!done.contains(rule)) { remaining.push_back(rule->name); }
      }
      return EF(
        "There is a dependency cycle somewhere, remaining rules: $",
        bee::join(remaining, "\n"));
    }
  }
  return sorted_rules;
}

} // namespace

// BuildNormalizer

BuildNormalizer::BuildNormalizer(
  const string& mbuild_name, const bee::FilePath& external_packages_dir)
    : _mbuild_name(mbuild_name), _external_packages_dir(external_packages_dir)
{}

OrError<NormalizedBuild> BuildNormalizer::normalize_build(
  const FilePath& repo_root_dir)
{
  vector<types::Profile> profiles;
  map<PackagePath, NormalizedRule::ptr> rules;

  auto read_rules = [this, &rules, &profiles, &repo_root_dir](
                      const bee::FilePath& root_package_dir,
                      bool include_profiles) -> OrError<> {
    bail(package_dirs, find_package_dirs(root_package_dir, _mbuild_name));
    for (const auto& dir : package_dirs) {
      bail(package_path, PackagePath::of_filesystem(root_package_dir, dir));
      auto mbuild_path = dir / _mbuild_name;
      bail(configs, MbuildParser::from_file(mbuild_path));
      for (const auto& rule : configs) {
        bail_unit(visit(
          [&]<class T>(const T& specific_rule) -> OrError<> {
            if constexpr (is_same_v<T, types::Profile>) {
              if (include_profiles) { profiles.push_back(specific_rule); }
              return bee::ok();
            } else if constexpr (is_same_v<T, types::ExternalPackage>) {
              return bee::ok();
            } else {
              auto rule = rules::Rule(specific_rule, package_path);
              auto name = rule.name();

              auto rel_dir = dir.relative_to(repo_root_dir);
              auto normalized = std::make_shared<NormalizedRule>(
                name, rel_dir, root_package_dir, rule);
              auto insert_res = rules.insert({name, normalized});
              if (!insert_res.second) {
                auto dup = rules.find(name);
                const auto& dup_loc = dup->second->location;
                string dup_message;
                if (dup_loc.has_value()) {
                  dup_message =
                    F("\n$: Package also defined here", dup_loc->hum());
                }
                print_error_with_loc(
                  rule.location(),
                  "Duplicated package name $ $",
                  name,
                  dup_message);
                return Error("Invalid mbuild");
              }
              return bee::ok();
            }
          },
          rule.value));
      }
    }
    return bee::ok();
  };

  bail_unit(read_rules(repo_root_dir, true));
  if (bee::FileSystem::exists(_external_packages_dir)) {
    bail_unit(read_rules(_external_packages_dir, false));
  }

  bail(sorted, top_sort(rules));
  return NormalizedBuild{
    .normalized_rules = std::move(sorted),
    .profiles = std::move(profiles),
  };
}

} // namespace mellow
