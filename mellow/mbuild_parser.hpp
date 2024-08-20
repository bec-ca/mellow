#pragma once

#include <string>
#include <vector>

#include "mbuild_types.generated.hpp"

#include "bee/file_path.hpp"
#include "bee/or_error.hpp"

namespace mellow {

struct MbuildParser {
  using Rules = std::vector<types::Rule>;

  static bee::OrError<Rules> from_string(
    const bee::FilePath& filename, const std::string& content);

  static bee::OrError<Rules> from_file(const bee::FilePath& filename);

  static std::string to_string(const Rules& config);

  static bee::OrError<> to_file(
    const bee::FilePath& filename, const Rules& config);
};

} // namespace mellow
