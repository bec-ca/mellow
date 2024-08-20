#include "generate_build_config.hpp"

#include "build_config.generated.hpp"
#include "build_config.hpp"

#include "bee/file_path.hpp"
#include "bee/file_writer.hpp"
#include "bee/filesystem.hpp"
#include "bee/format_vector.hpp"
#include "bee/print.hpp"
#include "bee/string_util.hpp"
#include "bee/util.hpp"

using std::nullopt;
using std::optional;
using std::string;
using std::vector;

namespace mellow {

namespace {

optional<string> get_env(const string& variable_name)
{
  char* c_path = getenv(variable_name.data());
  if (c_path == nullptr) return nullopt;
  return string(c_path);
}

optional<bee::FilePath> resolve_executable_path(const bee::FilePath& name)
{
  auto path_opt = get_env("PATH");
  if (!path_opt.has_value()) { return nullopt; }
  const string& path = *path_opt;
  auto parts = bee::split(path, ":");

  for (const auto& part : parts) {
    auto candidate = bee::FilePath(part) / name;
    if (bee::FileSystem::exists(candidate)) { return candidate; }
  }

  return nullopt;
}

bee::FilePath maybe_expand(const bee::FilePath& path)
{
  if (path.empty() || path.is_absolute()) { return path; }
  if (auto resolved = resolve_executable_path(path)) { return *resolved; }
  return path;
}

bee::FilePath get_cpp_compiler(const optional<bee::FilePath>& default_compiler)
{
  if (default_compiler.has_value()) {
    auto expanded = resolve_executable_path(*default_compiler);
    if (expanded.has_value()) {
      return *expanded;
    } else {
      PE("Default compiler '$' not found in $PATH", *default_compiler);
    }
  }

  if (auto cxx_variable = get_env("CXX")) {
    auto expanded = resolve_executable_path(bee::FilePath(*cxx_variable));
    if (expanded.has_value()) {
      return *expanded;
    } else {
      PE("Compiler in CXX env variable not found in $PATH", *default_compiler);
    }
  }

  return maybe_expand(bee::FilePath("g++"));
}

void append_from_variable(const string& variable_name, vector<string>& output)
{
  if (auto values = get_env(variable_name)) {
    bee::concat(output, bee::split_space(*values));
  }
}

vector<string> get_cpp_flags()
{
  vector<string> flags;
  append_from_variable("CXXFLAGS", flags);
  append_from_variable("CPPFLAGS", flags);
  return flags;
}

vector<string> get_ld_flags()
{
  vector<string> flags;
  append_from_variable("LDFLAGS", flags);
  append_from_variable("LDLIBS", flags);
  return flags;
}

BuildConfig generate_config(const GenerateBuildConfig::Args& args)
{
  BuildConfig config;
  auto cpp_compiler = get_cpp_compiler(args.default_cpp_compiler);
  auto cpp_flags = get_cpp_flags();
  auto ld_flags = get_ld_flags();
  P("CPP compiler: $", cpp_compiler);
  if (!cpp_flags.empty()) { P("CPP flags: $", cpp_flags); }
  if (!ld_flags.empty()) { P("LD flags: $", ld_flags); }
  config.rules.push_back({generated::Cpp{
    .compiler = cpp_compiler,
    .cpp_flags = cpp_flags,
    .ld_flags = ld_flags,
  }});
  return config;
}

} // namespace

bee::OrError<> GenerateBuildConfig::generate(
  const bee::FilePath& output, const Args& args)
{
  auto config = generate_config(args);
  P("Config written to $", output);
  return bee::FileWriter::write_file(
    output, yasf::ser(config)->to_string_hum());
}

} // namespace mellow
