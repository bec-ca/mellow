#pragma once

#include <filesystem>
#include <optional>
#include <vector>

#include "bee/error.hpp"
#include "bee/file_path.hpp"

namespace mellow {

struct BuildEngine {
  struct Args {
    bee::FilePath root_source_dir;
    std::string mbuild_name;
    std::string build_config;
    std::optional<std::string> profile_name;
    bee::FilePath output_dir_base;
    bee::FilePath external_packages_dir;
    bool verbose;
    bool force_build;
    bool update_test_output;
  };

  static bee::OrError<bee::Unit> build(const Args& args);
};

} // namespace mellow
