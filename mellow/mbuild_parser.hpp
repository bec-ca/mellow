#pragma once

#include <string>
#include <vector>

#include "generated_mbuild_parser.hpp"

#include "bee/error.hpp"
#include "bee/file_path.hpp"

namespace mellow {

namespace gmp = generated_mbuild_parser;

struct MbuildParser {
  using Rules = std::vector<gmp::Rule>;

  static bee::OrError<Rules> from_string(
    const bee::FilePath& filename, const std::string& content);

  static bee::OrError<Rules> from_file(const bee::FilePath& filename);

  static std::string to_string(const Rules& config);

  static bee::OrError<> to_file(
    const bee::FilePath& filename, const Rules& config);
};

} // namespace mellow
