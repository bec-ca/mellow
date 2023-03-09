#pragma once

#include "generated_build_hash.hpp"
#include "package_path.hpp"

#include "bee/file_path.hpp"

#include <filesystem>
#include <set>
#include <vector>

namespace mellow {

struct HashChecker {
  static HashChecker create(
    const bee::FilePath& hash_filename,
    const std::set<bee::FilePath>& inputs,
    const std::set<bee::FilePath>& outputs,
    const std::string& non_file_inputs_key);

  bool is_up_to_date();

  void write_updated_hashes();

 private:
  HashChecker(
    bee::FilePath hash_filename,
    std::set<bee::FilePath> inputs,
    std::set<bee::FilePath> outputs,
    std::string current_flags_hash);

  bee::FilePath _hash_filename;
  std::set<bee::FilePath> _inputs;
  std::set<bee::FilePath> _outputs;
  std::string _non_file_inputs_key;

  std::optional<generated_build_hash::TaskHash> _current_hashes_if_up_to_date;
};

} // namespace mellow
