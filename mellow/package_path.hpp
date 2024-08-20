#pragma once

#include <compare>
#include <string>
#include <string_view>
#include <vector>

#include "bee/file_path.hpp"
#include "bee/or_error.hpp"

namespace mellow {

struct PackagePath {
  PackagePath append_no_sep(const std::string_view& s) const;

  PackagePath append(const std::string_view& tail) const;
  void append_inplace(const std::string_view& tail);

  PackagePath operator/(const std::string_view& tail) const;
  PackagePath operator+(const std::string_view& tail) const;

  std::strong_ordering operator<=>(const PackagePath& other) const = default;

  static PackagePath root();

  bee::FilePath to_filesystem(const bee::FilePath& root_package_dir) const;

  std::string to_string() const;

  static bee::OrError<PackagePath> of_string(const std::string_view& str);

  static bee::OrError<PackagePath> of_filesystem(
    const bee::FilePath& root_package_dir, const bee::FilePath& path);

  const std::string& last() const;

  bool is_absolute() const;

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
