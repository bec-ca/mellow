#include "build_normalizer.hpp"

#include "bee/file_path.hpp"
#include "generated_mbuild_parser.hpp"
#include "mbuild_parser.hpp"
#include "package_path.hpp"

#include "bee/filesystem.hpp"
#include "bee/util.hpp"

#include <filesystem>
#include <map>
#include <string>

using bee::Error;
using bee::FilePath;
using bee::is_one_of_v;
using bee::OrError;
using bee::Unit;
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
    if (ignore_dirs.contains(p.filename()) || p.filename().starts_with(".")) {
      continue;
    }
    bail(subdirs, find_package_dirs(p, mbuild_name));
    concat(output, subdirs);
  }
  for (const auto& p : regular_files.regular_files) {
    if (p.filename() == mbuild_name) { output.push_back(p.parent_path()); }
  }

  return output;
}

template <class... Ts>
static Error error_with_loc(
  const optional<yasf::Location>& loc, const char* format, Ts&&... args)
{
  auto msg = bee::format(format, std::forward<Ts>(args)...);
  if (loc.has_value()) {
    return Error::format("$: $", loc->hum(), msg);
  } else {
    return Error(msg);
  }
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
          return error_with_loc(
            rule->location,
            "Rule '$' depends on unknown rule '$'",
            rule->name,
            dep);
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
            return error_with_loc(
              rule->location,
              "Rule '$' depends on unknown lib '$'",
              rule->name,
              lib);
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
      return Error::format("There is a dependency cycle somewhere");
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
  const FilePath& root_package_dir)
{
  vector<gmp::Profile> profiles;
  map<PackagePath, NormalizedRule::ptr> rules;

  auto read_rules = [this, &rules, &profiles](
                      const bee::FilePath root_source_dir,
                      bool include_profiles) -> OrError<Unit> {
    bail(package_dirs, find_package_dirs(root_source_dir, _mbuild_name));
    for (const auto& dir : package_dirs) {
      bail(package_path, PackagePath::of_filesystem(root_source_dir, dir));
      auto mbuild_path = dir / _mbuild_name;
      bail(configs, MbuildParser::from_file(mbuild_path));
      for (const auto& rule : configs) {
        bail_unit(visit(
          [&]<class T>(const T& specific_rule) -> OrError<Unit> {
            if constexpr (is_same_v<T, gmp::Profile>) {
              if (include_profiles) { profiles.push_back(specific_rule); }
              return bee::ok();
            } else if constexpr (is_same_v<T, gmp::ExternalPackage>) {
              return bee::ok();
            } else {
              auto rule = rules::Rule(specific_rule, package_path);
              auto name = rule.name();

              auto normalized =
                NormalizedRule::ptr(new NormalizedRule(name, dir, rule));
              auto insert_res = rules.insert({name, normalized});
              if (!insert_res.second) {
                auto dup = rules.find(name);
                const auto& dup_loc = dup->second->location;
                string dup_message;
                if (dup_loc.has_value()) {
                  dup_message = bee::format(
                    "\n$: Package also defined here", dup_loc->hum());
                }
                return error_with_loc(
                  rule.location(),
                  "Duplicated package name $ $",
                  name,
                  dup_message);
              }
              return bee::ok();
            }
          },
          rule.value));
      }
    }
    return bee::ok();
  };

  bail_unit(read_rules(root_package_dir, true));
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
