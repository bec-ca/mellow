#include "genbuild.hpp"

#include <filesystem>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>

#include "generated_mbuild_parser.hpp"
#include "mbuild_parser.hpp"
#include "package_path.hpp"

#include "bee/error.hpp"
#include "bee/file_reader.hpp"
#include "bee/filesystem.hpp"
#include "bee/format_filesystem.hpp"
#include "bee/format_set.hpp"
#include "bee/format_vector.hpp"
#include "bee/sort.hpp"
#include "bee/string_util.hpp"
#include "bee/util.hpp"

namespace fs = std::filesystem;

using bee::always_false_v;
using bee::FilePath;
using bee::is_one_of_v;
using bee::OrError;

using bee::to_vector;
using std::is_same_v;
using std::map;
using std::optional;
using std::set;
using std::string;
using std::vector;

namespace mellow {
namespace {

bool is_cpp_header_extension(const string& ext)
{
  return ext == ".hpp" || ext == ".h";
}

bool is_cpp_source_extension(const string& ext)
{
  return ext == ".cpp" || ext == ".cc" || ext == ".c";
}

OrError<vector<FilePath>> list_files(const FilePath& dir)
{
  bail(files, bee::FileSystem::list_regular_files(dir));
  vector<FilePath> output;
  for (const auto& p : files) {
    auto extension = p.extension();
    if (
      is_cpp_header_extension(extension) ||
      is_cpp_source_extension(extension)) {
      output.push_back(p);
    }
  }
  return output;
}

struct Library {
  Library(const PackagePath& name) : name(name) {}

  void add_source(const string& source) { sources.insert(source); }

  void add_header(const string& header) { headers.insert(header); }

  void add_lib(const PackagePath& lib) { libs.insert(lib); }

  bool is_good() const { return !sources.empty() || !headers.empty(); }

  PackagePath name;
  set<string> sources;
  set<string> headers;
  set<PackagePath> libs;
};

struct Includes {
  vector<string> quote_includes;
  vector<string> angle_includes;
};

OrError<Includes> scan_includes(const FilePath& filename)
{
  bail(content, bee::FileReader::read_file(filename));
  auto lines = bee::split(content, "\n");

  Includes output;

  for (const auto& line : lines) {
    vector<string> parts = bee::split_space(line);
    if (parts.size() <= 1) { continue; }
    if (parts[0] != "#include") { continue; }
    string filename = parts[1];
    if (filename.size() < 2) continue;
    char kind = filename.front();
    filename = filename.substr(1, filename.size() - 2);

    if (kind == '"') {
      output.quote_includes.push_back(filename);
    } else if (kind == '<') {
      output.angle_includes.push_back(filename);
    }
  }
  return output;
}

} // namespace

FilePath get_repo_root_dir(FilePath dir)
{
  while (true) {
    if (bee::FileSystem::exists(dir / ".git")) { return dir; }
    auto parent = dir.parent();
    if (!bee::FileSystem::exists(parent / "mbuild") || dir == parent) {
      return dir;
    } else {
      dir = parent;
    }
  }
}

OrError<> GenBuild::run(
  const optional<string>& directory_opt,
  const optional<string>& mbuild_path_opt)
{
  auto dir = FilePath::of_string(fs::canonical(directory_opt.value_or(".")));
  auto mbuild_path =
    FilePath::of_string(mbuild_path_opt.value_or(dir.to_std_path() / "mbuild"));
  auto repo_root_dir = get_repo_root_dir(dir);

  bail(files, list_files(dir));

  // TODO: put the actual package path
  bail(package_path, PackagePath::of_filesystem(repo_root_dir, dir));

  auto to_relative = [&](const PackagePath& path) {
    return path.relative_to(package_path);
  };

  auto vec_to_relative = [&](const vector<PackagePath>& paths) {
    vector<string> out;
    for (const auto& path : paths) { out.push_back(to_relative(path)); }
    bee::sort(out);
    return out;
  };

  map<PackagePath, Library> libraries;
  vector<string> binaries;

  map<PackagePath, gmp::Rule> output_rules_map;

  map<PackagePath, gmp::Rule> pre_existing_rules;

  // read existing mbuild file

  auto rule_name = [](const auto& rule) {
    return std::visit([](const auto& rule) { return rule.name; }, rule.value);
  };

  map<PackagePath, gmp::SystemLib> system_libs;

  if (!bee::FileSystem::exists(mbuild_path)) {
    P("No build rules found, a new one will be created from scratch");
  } else {
    bail(existing_rules, MbuildParser::from_file(mbuild_path));
    for (const auto& rule : existing_rules) {
      pre_existing_rules.emplace(package_path / rule_name(rule), rule);
    }
    for (auto& rule : existing_rules) {
      bool should_keep = std::visit(
        [&system_libs, &package_path]<class T>(const T& rule) -> bool {
          if constexpr (is_same_v<T, gmp::SystemLib>) {
            system_libs.emplace(package_path / rule.name, rule);
            return true;
          } else if constexpr (is_one_of_v<
                                 T,
                                 gmp::CppBinary,
                                 gmp::CppLibrary>) {
            return false;
          } else if constexpr (is_one_of_v<
                                 T,
                                 gmp::GenRule,
                                 gmp::Profile,
                                 gmp::CppTest,
                                 gmp::ExternalPackage>) {
            return true;
          } else {
            static_assert(always_false_v<T> && "non exaustive visit");
          }
        },
        rule.value);
      if (should_keep) {
        output_rules_map.emplace(package_path / rule_name(rule), rule);
      }
    }
  }

  auto get_or_create_library = [&](const PackagePath& name) -> Library& {
    auto it = libraries.find(name);
    if (it == libraries.end()) {
      auto ret = libraries.emplace(name, Library(name));
      return ret.first->second;
    } else {
      return it->second;
    }
  };

  auto find_system_lib_by_header =
    [&](const string& header) -> const gmp::SystemLib* {
    for (const auto& [_, lib] : system_libs) {
      for (const auto& h : lib.provide_headers) {
        if (h == header) { return &lib; }
      }
    }
    return nullptr;
  };

  auto find_system_lib_by_name =
    [&](const PackagePath& name) -> const gmp::SystemLib* {
    auto it = system_libs.find(name);
    if (it == system_libs.end()) { return nullptr; }
    return &it->second;
  };

  for (const auto& file : files) {
    string extension = file.extension();
    auto lib_name = package_path / file.stem();

    auto& lib = get_or_create_library(lib_name);
    if (is_cpp_source_extension(extension)) {
      lib.add_source(file.filename());
    } else if (is_cpp_header_extension(extension)) {
      lib.add_header(file.filename());
    }

    bail(includes, scan_includes(file));
    for (const auto& header : includes.quote_includes) {
      // Guess the library deps based in include headers. If the header path
      // has at least on '/' then it is considered an absolute import
      // TODO: check whether the file actually exist
      auto p = fs::path(header);
      p.replace_extension("");
      PackagePath root =
        p.has_parent_path() ? PackagePath::root() : package_path;
      PackagePath path = root / p;
      // Source files include the header counterpart, but that is not a library
      // dep
      if (path != lib.name) { lib.add_lib(path); }
    }

    for (const auto& header : includes.angle_includes) {
      if (auto system_lib = find_system_lib_by_header(header)) {
        lib.add_lib(package_path / system_lib->name);
      }
    }

    if (bee::ends_with(lib_name.last(), "_main")) {
      binaries.push_back(lib_name.last().substr(0, lib_name.last().size() - 5));
    }
  }

  auto filter_libs = [&](const set<PackagePath>& libs) {
    set<PackagePath> output;
    for (const auto& lib : libs) {
      if (!lib.is_child_of(package_path)) {
        output.insert(lib);
      } else {
        auto it = libraries.find(lib);
        if (
          (it == libraries.end() || !it->second.is_good()) &&
          find_system_lib_by_name(lib) == nullptr)
          continue;
        output.insert(lib);
      }
    }
    return output;
  };

  auto find_rule = [](
                     const PackagePath& name,
                     map<PackagePath, gmp::Rule>& rules) -> gmp::Rule* {
    auto it = rules.find(name);
    if (it == rules.end()) { return nullptr; }
    return &it->second;
  };

  auto find_from_pre_existing = [&](const PackagePath& name) {
    return find_rule(name, pre_existing_rules);
  };

  auto find_from_rules = [&](const PackagePath& name) -> gmp::Rule* {
    return find_rule(name, output_rules_map);
  };

  auto merge_rules = [&](const gmp::Rule& new_rule, const gmp::Rule& orig) {
    auto output = new_rule;
    std::visit(
      [&]<class T>(T& rule) {
        std::visit(
          [&]<class U>(const U& orig) {
            if constexpr (!is_same_v<T, U>) {
              throw std::runtime_error("Unexpected mismatch rule type");
            } else if constexpr (is_same_v<T, gmp::Profile>) {
            } else if constexpr (is_same_v<T, gmp::CppBinary>) {
              rule.ld_flags = orig.ld_flags;
            } else if constexpr (is_same_v<T, gmp::CppLibrary>) {
              rule.ld_flags = orig.ld_flags;
            } else if constexpr (is_same_v<T, gmp::CppTest>) {
              rule.os_filter = orig.os_filter;
            } else if constexpr (is_same_v<T, gmp::GenRule>) {
            } else if constexpr (is_same_v<T, gmp::SystemLib>) {
            } else if constexpr (is_same_v<T, gmp::ExternalPackage>) {
            } else {
              static_assert(always_false_v<T> && "non exaustive visit");
            }
          },
          orig.value);
      },
      output.value);
    return output;
  };

  auto replace_rule = [&](const PackagePath& name, gmp::Rule&& new_rule) {
    if (auto to_replace = find_from_rules(name)) {
      *to_replace = merge_rules(new_rule, *to_replace);
    } else if (auto to_replace = find_from_pre_existing(name)) {
      output_rules_map.emplace(name, merge_rules(new_rule, *to_replace));
    } else {
      output_rules_map.emplace(name, std::move(new_rule));
    }
  };

  for (const auto& kv : libraries) {
    const auto& lib = kv.second;
    if (bee::ends_with(lib.name.last(), "_test")) {
      auto cpp_test = gmp::CppTest{
        .name = lib.name.last(),
        .sources = to_vector(lib.sources),
        .libs = vec_to_relative(to_vector(filter_libs(lib.libs))),
        .output = lib.name.last() + ".out",
      };
      replace_rule(lib.name, gmp::Rule(std::move(cpp_test)));
    } else {
      if (!lib.is_good()) { continue; }
      auto cpp_library = gmp::CppLibrary{
        .name = lib.name.last(),
        .sources = to_vector(lib.sources),
        .headers = to_vector(lib.headers),
        .libs = vec_to_relative(to_vector(filter_libs(lib.libs))),
        .ld_flags = {},
      };
      replace_rule(lib.name, gmp::Rule(cpp_library));
    }
  }

  for (const auto& name : binaries) {
    auto cpp_binary = gmp::CppBinary{
      .name = name,
      .sources = {},
      .libs = {F("$_main", name)},
      .ld_flags = {},
    };
    replace_rule(package_path / name, gmp::Rule(cpp_binary));
  }

  vector<gmp::Rule> rules;
  for (auto& [_, rule] : output_rules_map) { rules.push_back(rule); }
  return MbuildParser::to_file(mbuild_path, rules);
}

} // namespace mellow
