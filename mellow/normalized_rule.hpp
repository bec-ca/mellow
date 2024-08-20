#pragma once

#include <set>

#include "build_rules.hpp"
#include "package_path.hpp"

#include "yasf/location.hpp"

namespace mellow {

struct NormalizedRule {
 public:
  using ptr = std::shared_ptr<NormalizedRule>;

  NormalizedRule(
    const PackagePath& rule_name,
    const bee::FilePath& package_dir,
    const bee::FilePath& root_package_dir,
    const rules::Rule& rule);

  ~NormalizedRule();

  const PackagePath name;

  // The path to the package that contains this rule
  const PackagePath package_name;

  // Directory where this package is found relative to the src root dir
  const bee::FilePath package_dir;

  // The directory of the root package for this repo
  const bee::FilePath root_package_dir;

  // All rules that are explicit dependencies of this rule, eg, libs, other
  // binaries. This determines the order in which the rules are processed
  // (which is different from the order they run).
  const std::set<PackagePath> deps;

  // Direct and indirect lib dependencies
  std::set<ptr> transitive_libs = {};

  const std::optional<yasf::Location> location;

  std::set<PackagePath> libs() const;

  std::set<bee::FilePath> headers() const;
  std::set<bee::FilePath> sources() const;
  std::set<bee::FilePath> data() const;
  std::optional<PackagePath> system_lib_config() const;

  std::vector<std::string> cpp_flags() const;
  std::vector<std::string> ld_flags() const;
  std::optional<PackagePath> output_cpp_object() const;

  std::vector<types::OS> os_filter() const;

  types::Rule raw_rule() const;

 private:
  const rules::Rule _rule;
};

} // namespace mellow
