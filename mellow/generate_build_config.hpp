#pragma once

#include "bee/error.hpp"

#include <filesystem>

namespace mellow {

struct GenerateBuildConfig {
  struct Args {
    std::optional<std::string> default_cpp_compiler = std::nullopt;
  };

  static bee::OrError<bee::Unit> generate(
    const std::filesystem::path& output, const Args& args);
};

} // namespace mellow
