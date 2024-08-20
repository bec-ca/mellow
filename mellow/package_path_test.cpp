#include <string>

#include "package_path.hpp"

#include "bee/format.hpp"
#include "bee/format_filesystem.hpp"
#include "bee/testing.hpp"

using bee::FilePath;

using std::string;

namespace mellow {
namespace {

TEST(basic)
{
  auto root_package_dir = FilePath(".");
  must(package_path, PackagePath::of_string("/foo"));
  P("package_dir:$ package_path:$", root_package_dir, package_path);
  auto to_filesystem = package_path.to_filesystem(root_package_dir);
  P("package_dir.to_filesystem:$", to_filesystem);
  auto of_filesystem =
    PackagePath::of_filesystem(root_package_dir, to_filesystem);
  P("package_dir.of_filesystem:$", of_filesystem);
}

TEST(of_filesystem)
{
  auto of_filesystem = [](const string& dir, const string& tail) {
    return PackagePath::of_filesystem(FilePath(dir), FilePath(tail));
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
    return pkg.to_filesystem(FilePath(dir));
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
  auto t = [](const char* path, const char* tail) {
    must(pkg, PackagePath::of_string(path));
    auto out = bee::try_with([&]() { return pkg / tail; });
    P("'$' '$' -> '$'", path, tail, out);
  };
  t("/foo/bar", "foo");
  t("/foo/bar", "/foo");
  t("/foo/bar", "./foo");
  t("/foo/bar", "././foo");
  t("/foo/bar", "../foo");
}

TEST(append_no_sep)
{
  auto t = [](const char* path, const char* tail) {
    must(pkg, PackagePath::of_string(path));
    auto out = pkg.append_no_sep(tail);
    P("'$' '$' -> '$'", path, tail, out);
  };
  t("/foo/bar", "foo");
  t("/foo/bar", "/foo");
  t("/foo/bar", "./foo");
  t("/foo/bar", "././foo");
  t("/foo/bar", "../foo");
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
    auto p2 = bee::FilePath(s2);
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
