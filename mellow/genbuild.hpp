#pragma once

#include <optional>
#include <string>

#include "bee/file_path.hpp"
#include "bee/or_error.hpp"

namespace mellow {

struct GenBuild {
  static bee::OrError<> run(
    const std::optional<bee::FilePath>& directory,
    const std::optional<bee::FilePath>& output_mbuild);
};

} // namespace mellow
