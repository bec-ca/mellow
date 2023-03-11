#include "package_path.hpp"

#include <deque>
#include <filesystem>
#include <string>
#include <vector>

#include "bee/error.hpp"
#include "bee/file_path.hpp"
#include "bee/format_vector.hpp"
#include "bee/string_util.hpp"
#include "bee/util.hpp"

using bee::FilePath;
using std::string;
using std::vector;

namespace mellow {
namespace {

bool is_root(const string& path)
{
  return !path.empty() && path.front() == '/';
}

bool ends_with_slash(const string& path)
{
  return !path.empty() && path.back() == '/';
}

vector<string> split_path(const string& path)
{
  vector<string> parts = bee::split(path, "/");
  vector<string> filtered_parts;

  for (const string& part : parts) {
    if (part == "." || part.empty()) continue;
    filtered_parts.push_back(part);
  }
  if (filtered_parts.size() > 1 && filtered_parts.back().empty()) {
    filtered_parts.pop_back();
  }
  return filtered_parts;
}

bee::OrError<vector<string>> split_path_for_package(const string& path)
{
  if (!path.empty() && path.front() != '/') {
    return bee::Error("Package name must start with a slash");
  }
  size_t start_index = 0;
  for (; start_index < path.size() && path[start_index] == '/'; start_index++) {
  }
  auto parts = split_path(path.substr(start_index));

  for (const auto& part : parts) {
    if (part == "..") {
      return bee::Error("'..' is not allowed in package name");
    }
  }
  return parts;
}

bool is_prefix_of(const vector<string>& prefix, const vector<string>& vec)
{
  if (prefix.size() > vec.size()) return false;
  for (size_t i = 0; i < prefix.size(); i++) {
    if (prefix[i] != vec[i]) { return false; }
  }
  return true;
}

} // namespace

PackagePath PackagePath::append_no_sep(const std::string& s) const
{
  // What if s has a separator?
  PackagePath copy = *this;
  if (copy._parts.empty()) {
    copy._parts.push_back(s);
  } else {
    copy._parts.back() += s;
  }
  return copy;
}

PackagePath PackagePath::append(const std::string& tail) const
{
  if (is_root(tail)) {
    must(out, of_string(tail));
    return out;
  }
  PackagePath copy = *this;
  copy.append_inplace(tail);
  return copy;
}

void PackagePath::append_inplace(const std::string& tail)
{
  auto parts = split_path(tail);
  if (!parts.empty() && parts[0].empty()) {
  } else {
    bee::concat(_parts, std::move(parts));
  }
}

PackagePath PackagePath::operator/(const string& tail) const
{
  return append(tail);
}

// Package path should always be rooted.
PackagePath PackagePath::root() { return PackagePath(vector<string>()); }

FilePath PackagePath::to_filesystem(const FilePath& root_package_dir) const
{
  string output = root_package_dir.to_string();
  for (const auto& part : _parts) {
    if (!ends_with_slash(output)) output += '/';
    output += part;
  }
  return FilePath::of_string(output);
}

bee::OrError<PackagePath> PackagePath::of_filesystem(
  const FilePath& root_package_dir, const FilePath& path)
{
  bool root_is_root = is_root(root_package_dir.to_string());
  bool path_is_root = is_root(path.to_string());

  if (root_is_root != path_is_root) {
    return bee::Error("of_filesystem() requires that neither inputs are root "
                      "or that both are.");
  }

  auto root_parts = split_path(root_package_dir.to_string());
  auto path_parts = split_path(path.to_string());

  if (!is_prefix_of(root_parts, path_parts)) {
    return bee::Error::format(
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

PackagePath::PackagePath(vector<string>&& parts) : _parts(std::move(parts)) {}

string PackagePath::to_string() const { return "/" + bee::join(_parts, "/"); }

bee::OrError<PackagePath> PackagePath::of_string(const string& path)
{
  bail(parts, split_path_for_package(path));
  return PackagePath(std::move(parts));
}

const string& PackagePath::last() const { return _parts.back(); }

const string& PackagePath::operator[](int idx) const { return _parts.at(idx); }
const string& PackagePath::at(int idx) const { return _parts.at(idx); }

int PackagePath::size() const { return _parts.size(); }

bool PackagePath::is_child_of(const PackagePath& parent) const
{
  if (size() <= parent.size()) return false;
  for (int i = 0; i < parent.size(); i++) {
    if (at(i) != parent.at(i)) { return false; }
  }
  return true;
}

string PackagePath::relative_to(const PackagePath& parent) const
{
  if (*this == parent) { return "./"; }
  if (!is_child_of(parent)) { return to_string(); }
  vector<string> tail(_parts.begin() + parent.size(), _parts.end());
  return bee::join(tail, "/");
}

PackagePath PackagePath::parent() const
{
  assert(!_parts.empty());
  return PackagePath(vector(_parts.begin(), _parts.end() - 1));
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
