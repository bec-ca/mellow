#pragma once

#include <filesystem>
#include <set>
#include <string>

#include "normalized_rule.hpp"
#include "package_path.hpp"

#include "bee/error.hpp"

namespace mellow {

struct NormalizedBuild {
  std::vector<NormalizedRule::ptr> normalized_rules;
  std::vector<gmp::Profile> profiles;
};

struct BuildNormalizer {
  BuildNormalizer(
    const std::string& mbuild_name, const bee::FilePath& external_packages_dir);

  bee::OrError<NormalizedBuild> normalize_build(
    const bee::FilePath& root_package);

 private:
  std::string _mbuild_name;
  bee::FilePath _external_packages_dir;
};

} // namespace mellow
