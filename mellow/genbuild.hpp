#pragma once

#include "bee/error.hpp"

#include <optional>
#include <string>

namespace mellow {

struct GenBuild {
  static bee::OrError<bee::Unit> run(
    const std::optional<std::string>& directory,
    const std::optional<std::string>& output_mbuild);
};

} // namespace mellow
