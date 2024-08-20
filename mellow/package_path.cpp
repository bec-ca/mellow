#include "package_path.hpp"

#include <deque>
#include <string>
#include <vector>

#include "bee/file_path.hpp"
#include "bee/format_vector.hpp"
#include "bee/or_error.hpp"
#include "bee/string_util.hpp"
#include "bee/util.hpp"

namespace mellow {
namespace {

bool is_root_str(const std::string_view& path)
{
  return !path.empty() && path.front() == '/';
}

bool ends_with_slash(const std::string_view& path)
{
  return !path.empty() && path.back() == '/';
}

bee::OrError<std::vector<std::string>> split_path(const std::string_view& path)
{
  auto parts = bee::split(path, "/");
  std::vector<std::string> filtered_parts;

  for (auto& part : parts) {
    if (part == "." || part.empty()) continue;
    if (part == "..") { return EF("'..' is not allowed in package name"); }
    filtered_parts.push_back(std::move(part));
  }
  if (filtered_parts.size() > 1 && filtered_parts.back().empty()) {
    filtered_parts.pop_back();
  }
  return filtered_parts;
}

bee::OrError<std::vector<std::string>> split_path_for_package(
  const std::string_view& path)
{
  if (!path.empty() && path.front() != '/') {
    return bee::Error("Package name must start with a slash");
  }
  size_t start_index = 0;
  for (; start_index < path.size() && path[start_index] == '/'; start_index++) {
  }
  bail(parts, split_path(path.substr(start_index)));
  return parts;
}

bool is_prefix_of(
  const std::vector<std::string>& prefix, const std::vector<std::string>& vec)
{
  if (prefix.size() > vec.size()) return false;
  for (size_t i = 0; i < prefix.size(); i++) {
    if (prefix[i] != vec[i]) { return false; }
  }
  return true;
}

} // namespace

PackagePath PackagePath::append_no_sep(const std::string_view& s) const
{
  if (_parts.empty()) {
    must(out, of_string(s));
    return out;
  }
  PackagePath copy = *this;
  must(tail, split_path(copy._parts.back() + std::string(s)));
  copy._parts.pop_back();
  bee::concat(copy._parts, tail);
  return copy;
}

PackagePath PackagePath::append(const std::string_view& tail) const
{
  PackagePath copy = *this;
  copy.append_inplace(tail);
  return copy;
}

void PackagePath::append_inplace(const std::string_view& tail)
{
  if (is_root_str(tail)) {
    must(out, of_string(tail));
    *this = out;
  } else {
    must(parts, split_path(tail));
    bee::concat(_parts, std::move(parts));
  }
}

PackagePath PackagePath::operator/(const std::string_view& tail) const
{
  return append(tail);
}

PackagePath PackagePath::operator+(const std::string_view& tail) const
{
  return append_no_sep(tail);
}

// Package path should always be rooted.
PackagePath PackagePath::root() { return PackagePath({}); }

bee::FilePath PackagePath::to_filesystem(
  const bee::FilePath& root_package_dir) const
{
  auto output = root_package_dir.to_string();
  for (const auto& part : _parts) {
    if (!ends_with_slash(output)) output += '/';
    output += part;
  }
  return bee::FilePath(output);
}

bee::OrError<PackagePath> PackagePath::of_filesystem(
  const bee::FilePath& root_package_dir, const bee::FilePath& path)
{
  bool root_is_root = is_root_str(root_package_dir.to_string());
  bool path_is_root = is_root_str(path.to_string());

  if (root_is_root != path_is_root) {
    return bee::Error("of_filesystem() requires that neither inputs are root "
                      "or that both are.");
  }

  bail(root_parts, split_path(root_package_dir.to_string()));
  bail(path_parts, split_path(path.to_string()));

  if (!is_prefix_of(root_parts, path_parts)) {
    return bee::Error::fmt(
      "Path '$' is not a child of the root_package '$', $ $",
      path,
      root_package_dir,
      root_parts,
      path_parts);
  }

  PackagePath tail = PackagePath::root();
  for (size_t i = root_parts.size(); i < path_parts.size(); i++) {
    tail.append_inplace(path_parts[i]);
  }

  return tail;
}

PackagePath::PackagePath(std::vector<std::string>&& parts)
    : _parts(std::move(parts))
{}

std::string PackagePath::to_string() const
{
  return "/" + bee::join(_parts, "/");
}

bee::OrError<PackagePath> PackagePath::of_string(const std::string_view& path)
{
  bail(parts, split_path_for_package(path));
  return PackagePath(std::move(parts));
}

const std::string& PackagePath::last() const { return _parts.back(); }

const std::string& PackagePath::operator[](int idx) const
{
  return _parts.at(idx);
}
const std::string& PackagePath::at(int idx) const { return _parts.at(idx); }

int PackagePath::size() const { return _parts.size(); }

bool PackagePath::is_absolute() const
{
  return !_parts.empty() && _parts.front().empty();
}

bool PackagePath::is_child_of(const PackagePath& parent) const
{
  if (size() <= parent.size()) return false;
  for (int i = 0; i < parent.size(); i++) {
    if (at(i) != parent.at(i)) { return false; }
  }
  return true;
}

std::string PackagePath::relative_to(const PackagePath& parent) const
{
  if (*this == parent) { return "./"; }
  if (!is_child_of(parent)) { return to_string(); }
  std::vector<std::string> tail(_parts.begin() + parent.size(), _parts.end());
  return bee::join(tail, "/");
}

PackagePath PackagePath::parent() const
{
  assert(!_parts.empty());
  return PackagePath(std::vector(_parts.begin(), _parts.end() - 1));
}

bee::FilePath PackagePath::remove_suffix(const bee::FilePath& input_path) const
{
  auto path = input_path;
  auto parts = _parts;
  while (!parts.empty() && path.filename() == parts.back()) {
    path = path.parent();
    parts.pop_back();
  }
  return path;
}

} // namespace mellow
