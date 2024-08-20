#pragma once

#include <optional>

#include "bee/file_path.hpp"
#include "bee/or_error.hpp"

namespace mellow {

struct BuildEngine {
  struct Args {
    bee::FilePath repo_root_dir;
    std::string mbuild_name;
    bee::FilePath build_config;
    std::optional<std::string> profile_name;
    bee::FilePath output_dir_base;
    bee::FilePath external_packages_dir;
    bool verbose;
    bool force_build;
    bool force_test;
    bool update_test_output;
  };

  static bee::OrError<> build(const Args& args);
};

} // namespace mellow
