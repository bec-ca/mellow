#include "genbuild.hpp"

#include <map>
#include <optional>
#include <string>

#include "mbuild_parser.hpp"
#include "mbuild_types.generated.hpp"
#include "package_path.hpp"

#include "bee/file_reader.hpp"
#include "bee/filesystem.hpp"
#include "bee/or_error.hpp"
#include "bee/print.hpp"
#include "bee/ref.hpp"
#include "bee/sort.hpp"
#include "bee/string_util.hpp"
#include "bee/util.hpp"

using bee::FilePath;
using bee::is_one_of;
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

bool is_yasf_file_extension(const string& ext) { return ext == ".yasf"; }
bool is_exc_file_extension(const string& ext) { return ext == ".exc"; }

bool is_interesting_extension(const std::string& ext)
{
  return is_cpp_header_extension(ext) || is_cpp_source_extension(ext) ||
         is_yasf_file_extension(ext) || is_exc_file_extension(ext);
}

OrError<vector<FilePath>> list_files(const FilePath& dir)
{
  bail(
    files, bee::FileSystem::list_regular_files(dir, {.relative_path = true}));
  vector<FilePath> output;
  for (auto& p : files) {
    if (is_interesting_extension(p.extension())) {
      output.push_back(std::move(p));
    }
  }
  return output;
}

struct Target {
 public:
  Target(const PackagePath& name) : _name(name) {}

  const PackagePath& name() const { return _name; }

 private:
  PackagePath _name;
};

struct Library : Target {
  Library(const PackagePath& name) : Target(name) {}

  void add_source(const string& source) { sources.insert(source); }

  void add_header(const string& header) { headers.insert(header); }

  void add_lib(const PackagePath& lib) { libs.insert(lib); }

  bool is_good() const { return !sources.empty() || !headers.empty(); }

  set<string> sources;
  set<string> headers;
  set<PackagePath> libs;
};

struct BasicGenRule : Target {
 public:
  BasicGenRule(const PackagePath& name, const bee::FilePath& source)
      : Target(name), _source(source)
  {}

  const bee::FilePath& source() const { return _source; }

  const std::vector<std::string> outputs() const
  {
    const auto stem = _source.stem();
    return {
      stem + ".generated.cpp",
      stem + ".generated.hpp",
    };
  }

 private:
  bee::FilePath _source;
  bee::FilePath _cpp_source;
  bee::FilePath _cpp_header;
};

struct YasfRule : BasicGenRule {
 public:
  using BasicGenRule::BasicGenRule;
};

struct ExcRule : BasicGenRule {
 public:
  using BasicGenRule::BasicGenRule;
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
  const optional<FilePath>& directory_opt,
  const optional<FilePath>& mbuild_path_opt)
{
  bail(dir, bee::FileSystem::canonical(directory_opt.value_or(FilePath("."))));
  auto mbuild_path = mbuild_path_opt.value_or(dir / "mbuild");
  auto repo_root_dir = get_repo_root_dir(dir);

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
  vector<YasfRule> yasf_rules;
  vector<ExcRule> exc_rules;
  vector<string> binaries;

  map<PackagePath, types::Rule> output_rules_map;

  map<PackagePath, types::Rule> pre_existing_rules;

  // read existing mbuild file

  auto rule_name = [](const auto& rule) {
    return std::visit([](const auto& rule) { return rule.name; }, rule.value);
  };

  map<PackagePath, types::SystemLib> system_libs;

  if (!bee::FileSystem::exists(mbuild_path)) {
    P("No build rules found, a new one will be created from scratch");
  } else {
    bail(existing_rules, MbuildParser::from_file(mbuild_path));
    for (const auto& rule : existing_rules) {
      pre_existing_rules.emplace(package_path / rule_name(rule), rule);
    }
    for (const auto& rule : existing_rules) {
      const bool should_keep = rule.visit(
        [&system_libs, &package_path]<class T>(const T& rule) -> bool {
          if constexpr (is_same_v<T, types::SystemLib>) {
            system_libs.emplace(package_path / rule.name, rule);
            return true;
          } else if constexpr (
            is_one_of<T, types::CppBinary, types::CppLibrary, types::CppTest>) {
            return false;
          } else if constexpr (is_one_of<
                                 T,
                                 types::Profile,
                                 types::ExternalPackage>) {
            return true;
          } else if constexpr (is_one_of<T, types::GenRule>) {
            bool has_yasf_data =
              rule.data.size() == 1 && rule.data.at(0).ends_with(".yasf");
            bool has_exc_data =
              rule.data.size() == 1 && rule.data.at(0).ends_with(".exc");
            return !has_yasf_data && !has_exc_data;
          } else {
            static_assert(bee::always_false<T> && "non exhaustive visit");
          }
        });
      if (should_keep) {
        output_rules_map.emplace(package_path / rule_name(rule), rule);
      }
    }
  }

  auto get_or_create_library = [&](const PackagePath& name) {
    auto it = libraries.find(name);
    if (it == libraries.end()) { it = libraries.emplace(name, name).first; }
    return bee::ref(it->second);
  };

  auto find_system_lib_by_header =
    [&](const string& header) -> bee::nref<const types::SystemLib> {
    for (const auto& [_, lib] : system_libs) {
      for (const auto& h : lib.provide_headers) {
        if (h == header) { return lib; }
      }
    }
    return nullptr;
  };

  auto find_system_lib_by_name =
    [&](const PackagePath& name) -> bee::nref<const types::SystemLib> {
    auto it = system_libs.find(name);
    if (it == system_libs.end()) { return nullptr; }
    return it->second;
  };

  bail(files, list_files(dir));
  for (const auto& file : files) {
    const string extension = file.extension();
    if (is_yasf_file_extension(extension)) {
      auto name = package_path / file.stem() + "_yasf_codegen";
      yasf_rules.emplace_back(name, file);
    } else if (is_exc_file_extension(extension)) {
      auto name = package_path / file.stem() + "_exc_codegen";
      exc_rules.emplace_back(name, file);
    } else {
      const auto lib_name = package_path / file.stem();

      auto lib = get_or_create_library(lib_name);
      if (is_cpp_source_extension(extension)) {
        lib->add_source(file.to_string());
      } else if (is_cpp_header_extension(extension)) {
        lib->add_header(file.to_string());
      } else {
        raise_error("Unexpected file seen: $", file);
      }

      bail(includes, scan_includes(dir / file));
      for (const auto& header : includes.quote_includes) {
        // Guess the library deps based on include headers. If the header path
        // has at least on '/' then it is considered an absolute import
        // TODO: check whether the file actually exist
        FilePath p(header);
        p = p.remove_extension();
        PackagePath root = p.has_parent() ? PackagePath::root() : package_path;
        auto path = root / p.to_string();
        // Source files include the header counterpart, but
        // that is not a library dep
        if (path != lib->name()) { lib->add_lib(path); }
      }

      for (const auto& header : includes.angle_includes) {
        if (auto system_lib = find_system_lib_by_header(header)) {
          lib->add_lib(package_path / system_lib->name);
        }
      }

      if (auto n = bee::remove_suffix(lib_name.last(), "_main")) {
        binaries.push_back(std::move(*n));
      }
    }
  }

  auto filter_libs = [&](const set<PackagePath>& libs) {
    set<PackagePath> output;
    for (const auto& lib : libs) {
      bool is_system_lib = find_system_lib_by_name(lib) != nullptr;
      bool different_package = lib.parent() != package_path;
      auto it = libraries.find(lib);
      bool is_lib_good = it != libraries.end() && it->second.is_good();
      if (is_system_lib || different_package || is_lib_good) {
        output.insert(lib);
      }
    }
    return output;
  };

  auto find_rule =
    [](
      const PackagePath& name,
      map<PackagePath, types::Rule>& rules) -> bee::nref<types::Rule> {
    auto it = rules.find(name);
    if (it == rules.end()) { return nullptr; }
    return it->second;
  };

  auto find_from_pre_existing = [&](const PackagePath& name) {
    return find_rule(name, pre_existing_rules);
  };

  auto find_from_rules = [&](const PackagePath& name) {
    return find_rule(name, output_rules_map);
  };

  auto merge_rules = [&](const types::Rule& new_rule, const types::Rule& orig) {
    auto output = new_rule;
    output.visit([&]<class T>(T& rule) {
      orig.visit([&]<class U>(const U& orig) {
        if constexpr (!is_same_v<T, U>) {
          raise_error("Unexpected mismatch rule type");
        } else if constexpr (is_same_v<T, types::Profile>) {
        } else if constexpr (is_same_v<T, types::CppBinary>) {
          rule.ld_flags = orig.ld_flags;
        } else if constexpr (is_same_v<T, types::CppLibrary>) {
          rule.ld_flags = orig.ld_flags;
        } else if constexpr (is_same_v<T, types::CppTest>) {
          rule.os_filter = orig.os_filter;
        } else if constexpr (is_same_v<T, types::GenRule>) {
        } else if constexpr (is_same_v<T, types::SystemLib>) {
        } else if constexpr (is_same_v<T, types::ExternalPackage>) {
        } else {
          static_assert(bee::always_false<T> && "non exhaustive visit");
        }
      });
    });
    return output;
  };

  auto replace_rule = [&](const PackagePath& name, types::Rule&& new_rule) {
    if (auto to_replace = find_from_rules(name)) {
      *to_replace = merge_rules(new_rule, *to_replace);
    } else if (auto to_replace = find_from_pre_existing(name)) {
      output_rules_map.emplace(name, merge_rules(new_rule, *to_replace));
    } else {
      output_rules_map.emplace(name, std::move(new_rule));
    }
  };

  for (const auto& [_, lib] : libraries) {
    if (lib.name().last().ends_with("_test")) {
      auto cpp_test = types::CppTest{
        .name = lib.name().last(),
        .sources = to_vector(lib.sources),
        .libs = vec_to_relative(to_vector(filter_libs(lib.libs))),
        .output = lib.name().last() + ".out",
      };
      replace_rule(lib.name(), types::Rule(std::move(cpp_test)));
    } else {
      if (!lib.is_good()) { continue; }
      auto cpp_library = types::CppLibrary{
        .name = lib.name().last(),
        .sources = to_vector(lib.sources),
        .headers = to_vector(lib.headers),
        .libs = vec_to_relative(to_vector(filter_libs(lib.libs))),
        .ld_flags = {},
      };
      replace_rule(lib.name(), cpp_library);
    }
  }

  for (const auto& name : binaries) {
    auto cpp_binary = types::CppBinary{
      .name = name,
      .sources = {},
      .libs = {F("$_main", name)},
      .ld_flags = {},
    };
    replace_rule(package_path / name, cpp_binary);
  }

  for (const auto& rule : yasf_rules) {
    auto genrule = types::GenRule{
      .name = rule.name().last(),
      .binary = "/yasf/yasf_compiler",
      .flags = {"compile", rule.source().to_string()},
      .data = {rule.source().to_string()},
      .outputs = rule.outputs(),
    };
    replace_rule(rule.name(), genrule);
  }

  for (const auto& rule : exc_rules) {
    auto genrule = types::GenRule{
      .name = rule.name().last(),
      .binary = "/exc/exc",
      .flags = {"compile", rule.source().to_string()},
      .data = {rule.source().to_string()},
      .outputs = rule.outputs(),
    };
    replace_rule(rule.name(), genrule);
  }

  vector<types::Rule> rules;
  for (auto& [_, rule] : output_rules_map) { rules.push_back(rule); }
  return MbuildParser::to_file(mbuild_path, rules);
}

} // namespace mellow
