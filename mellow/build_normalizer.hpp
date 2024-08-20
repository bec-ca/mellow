#pragma once

#include <set>
#include <string>

#include "normalized_rule.hpp"
#include "package_path.hpp"

#include "bee/or_error.hpp"

namespace mellow {

struct NormalizedBuild {
  std::vector<NormalizedRule::ptr> normalized_rules;
  std::vector<types::Profile> profiles;
};

struct BuildNormalizer {
  BuildNormalizer(
    const std::string& mbuild_name, const bee::FilePath& external_packages_dir);

  bee::OrError<NormalizedBuild> normalize_build(
    const bee::FilePath& repo_root_dir);

 private:
  std::string _mbuild_name;
  bee::FilePath _external_packages_dir;
};

} // namespace mellow
