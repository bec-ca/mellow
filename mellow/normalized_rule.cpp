#include "normalized_rule.hpp"

using bee::FilePath;
using std::optional;
using std::set;
using std::string;

namespace mellow {

NormalizedRule::NormalizedRule(
  const PackagePath& rule_name,
  const bee::FilePath& package_dir,
  const rules::Rule& rule)
    : name(rule_name),
      package_name(rule_name.parent()),
      package_dir(package_dir),
      root_source_dir(package_name.remove_suffix(package_dir)),
      deps(rule.deps()),
      location(rule.location()),
      _rule(rule)
{}

NormalizedRule::~NormalizedRule() {}

set<FilePath> NormalizedRule::headers() const
{
  set<FilePath> output;
  for (const auto& h : _rule.headers()) { output.insert(package_dir / h); }
  return output;
}

set<FilePath> NormalizedRule::sources() const
{
  set<FilePath> output;
  for (const auto& s : _rule.sources()) { output.insert(package_dir / s); }
  return output;
}

std::vector<std::string> NormalizedRule::cpp_flags() const
{
  return _rule.cpp_flags();
}

std::vector<std::string> NormalizedRule::ld_flags() const
{
  return _rule.ld_flags();
}

optional<PackagePath> NormalizedRule::output_cpp_object() const
{
  return _rule.output_cpp_object();
}

gmp::Rule NormalizedRule::raw_rule() const { return _rule.raw(); }

std::optional<PackagePath> NormalizedRule::system_lib_config() const
{
  return _rule.system_lib_config();
}

std::vector<string> NormalizedRule::os_filter() const
{
  return _rule.os_filter();
}

std::set<PackagePath> NormalizedRule::libs() const { return _rule.libs(); }

} // namespace mellow
