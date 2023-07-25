#pragma once

#include <filesystem>

#include "bee/error.hpp"

namespace mellow {

struct GenerateBuildConfig {
  struct Args {
    std::optional<std::string> default_cpp_compiler = std::nullopt;
  };

  static bee::OrError<> generate(
    const std::filesystem::path& output, const Args& args);
};

} // namespace mellow
