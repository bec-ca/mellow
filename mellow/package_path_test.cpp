#include "package_path.hpp"

#include "bee/format.hpp"
#include "bee/format_filesystem.hpp"
#include "bee/testing.hpp"

#include <string>

using bee::FilePath;
using bee::print_line;
using std::string;

namespace mellow {
namespace {

TEST(basic)
{
  auto root_package_dir = FilePath::of_string(".");
  must(package_path, PackagePath::of_string("/foo"));
  print_line("package_dir:$ package_path:$", root_package_dir, package_path);
  auto to_filesystem = package_path.to_filesystem(root_package_dir);
  print_line("package_dir.to_filesystem:$", to_filesystem);
  auto of_filesystem =
    PackagePath::of_filesystem(root_package_dir, to_filesystem);
  print_line("package_dir.of_filesystem:$", of_filesystem);
}

TEST(of_filesystem)
{
  auto of_filesystem = [](const string& dir, const string& tail) {
    return PackagePath::of_filesystem(
      FilePath::of_string(dir), FilePath::of_string(tail));
  };

  PRINT_EXPR(of_filesystem(".", "./foo.out"));
  PRINT_EXPR(of_filesystem("./", "./foo.out"));
  PRINT_EXPR(of_filesystem("./core", "./core/foo.out"));
  PRINT_EXPR(of_filesystem("./core", "./core/bar/foo.out"));
  PRINT_EXPR(of_filesystem(
    "/usr/bin/home/user/foo", "/usr/bin/home/user/foo/core/bar/foo.out"));
  PRINT_EXPR(of_filesystem("./foo", "./bar"));
  PRINT_EXPR(of_filesystem("/foo", "./foo/bar"));
}

TEST(to_filesystem)
{
  auto to_filesystem = [](const string& path, const string& dir) -> FilePath {
    must(pkg, PackagePath::of_string(path));
    return pkg.to_filesystem(FilePath::of_string(dir));
  };
  PRINT_EXPR(to_filesystem("/foo/bar", "foo"));
  PRINT_EXPR(to_filesystem("/", "foo"));
  PRINT_EXPR(to_filesystem("/", "./"));
  PRINT_EXPR(to_filesystem("/foo", "./"));
}

TEST(of_string)
{
  PRINT_EXPR(PackagePath::of_string("/foo"));
  PRINT_EXPR(PackagePath::of_string("foo"));
  PRINT_EXPR(PackagePath::of_string("///foo"));
  PRINT_EXPR(PackagePath::of_string("/"));
  PRINT_EXPR(PackagePath::of_string("/foo/bar"));
  PRINT_EXPR(PackagePath::of_string("/foo/../bar"));
}

TEST(append)
{
  auto append = [](const string& path, const string& tail) -> PackagePath {
    must(pkg, PackagePath::of_string(path));
    return pkg / tail;
  };
  PRINT_EXPR(append("/foo/bar", "foo"));
  PRINT_EXPR(append("/foo/bar", "/foo"));
  PRINT_EXPR(append("/foo/bar", "./foo"));
  PRINT_EXPR(append("/foo/bar", "././foo"));
  PRINT_EXPR(append("/foo/bar", "../foo")); // TODO: this shouldn't be allowed
}

TEST(relative_to)
{
  auto append = [](const string& s1, const string& s2) {
    must(p1, PackagePath::of_string(s1));
    must(p2, PackagePath::of_string(s2));
    return p1.relative_to(p2);
  };
  PRINT_EXPR(append("/foo/bar", "/foo"));
  PRINT_EXPR(append("/foo/bar", "/foo/boo"));
  PRINT_EXPR(append("/foo/bar", "/"));
  PRINT_EXPR(append("/foo/bar", "/foo/bar"));
}

TEST(remove_suffix)
{
  auto remove_suffix = [](const string& s1, const string& s2) {
    must(p1, PackagePath::of_string(s1));
    auto p2 = bee::FilePath::of_string(s2);
    return p1.remove_suffix(p2);
  };
  PRINT_EXPR(remove_suffix("/foo/bar", "/other/foo/bar"));
  PRINT_EXPR(remove_suffix("/foo/bar", "/other/bar"));
  PRINT_EXPR(remove_suffix("/foo/bar", "/other/foo"));
  PRINT_EXPR(remove_suffix("/foo/bar", "/other"));
  PRINT_EXPR(remove_suffix("/foo/bar", "/"));
}
} // namespace
} // namespace mellow
