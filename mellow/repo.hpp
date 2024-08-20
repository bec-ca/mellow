#pragma once

#include "bee/file_path.hpp"
#include "bee/or_error.hpp"

namespace mellow {

struct Repo {
 public:
  static bee::OrError<bee::FilePath> root_dir(
    const bee::FilePath& starting_dir);
};

} // namespace mellow
