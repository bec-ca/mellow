#pragma once

#include "bee/file_path.hpp"
#include "bee/or_error.hpp"

namespace mellow {

struct GenerateBuildConfig {
  struct Args {
    std::optional<bee::FilePath> default_cpp_compiler = std::nullopt;
  };

  static bee::OrError<> generate(const bee::FilePath& output, const Args& args);
};

} // namespace mellow
