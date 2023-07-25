#pragma once

#include "bee/file_path.hpp"

namespace mellow {

struct Defaults {
  constexpr static char mbuild_name[] = "mbuild";

  static bee::FilePath external_packages_dir(const bee::FilePath& output_dir);

  static bee::FilePath output_dir();
};

} // namespace mellow
