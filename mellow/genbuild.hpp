#pragma once

#include <optional>
#include <string>

#include "bee/error.hpp"

namespace mellow {

struct GenBuild {
  static bee::OrError<> run(
    const std::optional<std::string>& directory,
    const std::optional<std::string>& output_mbuild);
};

} // namespace mellow
