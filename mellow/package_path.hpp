#pragma once

#include <compare>
#include <filesystem>
#include <string>
#include <vector>

#include "bee/error.hpp"
#include "bee/file_path.hpp"

namespace mellow {

struct PackagePath {
  PackagePath append_no_sep(const std::string& s) const;

  PackagePath append(const std::string& tail) const;
  void append_inplace(const std::string& tail);

  PackagePath operator/(const std::string& tail) const;

  std::strong_ordering operator<=>(const PackagePath& other) const = default;

  static PackagePath root();

  bee::FilePath to_filesystem(const bee::FilePath& root_package_dir) const;

  std::string to_string() const;

  static bee::OrError<PackagePath> of_string(const std::string& str);

  static bee::OrError<PackagePath> of_filesystem(
    const bee::FilePath& root_package_dir, const bee::FilePath& path);

  const std::string& last() const;

  bool is_child_of(const PackagePath& parent) const;

  std::string relative_to(const PackagePath& parent) const;

  PackagePath parent() const;

  const std::string& operator[](int idx) const;
  const std::string& at(int idx) const;

  int size() const;

  bee::FilePath remove_suffix(const bee::FilePath& path) const;

 private:
  explicit PackagePath(std::vector<std::string>&& parts);
  std::vector<std::string> _parts;
};

} // namespace mellow
