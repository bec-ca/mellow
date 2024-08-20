#pragma once

#include <variant>

#include "mbuild_types.generated.hpp"
#include "package_path.hpp"
#include "rule_templates.hpp"

#include "yasf/location.hpp"

namespace mellow::rules {

struct CppBinary : public BaseRule<types::CppBinary> {
 public:
  explicit CppBinary(const types::CppBinary&, const PackagePath&);
  ~CppBinary();
};

namespace details {

struct CppLibraryBase : public types::CppLibrary {
  CppLibraryBase(const types::CppLibrary&);
  std::optional<std::string> output_cpp_object() const;
};

} // namespace details

struct CppLibrary : public BaseRule<details::CppLibraryBase> {
 public:
  explicit CppLibrary(const types::CppLibrary&, const PackagePath&);
  ~CppLibrary();
};

struct CppTest : public BaseRule<types::CppTest> {
 public:
  explicit CppTest(const types::CppTest&, const PackagePath&);
  ~CppTest();

  const std::vector<types::OS>& os_filter() const;
  // std::vector<types::OS>& os_filter();
};

struct GenRule : public BaseRule<types::GenRule> {
 public:
  explicit GenRule(const types::GenRule&, const PackagePath&);
  ~GenRule();

  std::set<PackagePath> additional_deps() const;
};

namespace details {

struct SystemLibBase : public types::SystemLib {
  SystemLibBase(const types::SystemLib&);
  std::string system_lib_config() const;
};

} // namespace details

struct SystemLib : public BaseRule<details::SystemLibBase> {
 public:
  explicit SystemLib(const types::SystemLib&, const PackagePath&);
  ~SystemLib();

  std::vector<std::string> provide_headers() const;
};

struct ExternalPackage : public BaseRule<types::ExternalPackage> {
 public:
  explicit ExternalPackage(const types::ExternalPackage&);
  ~ExternalPackage();

  std::vector<std::string> provide_headers() const;
};

struct Rule {
  using rule_variant =
    std::variant<CppBinary, CppLibrary, CppTest, GenRule, SystemLib>;

  using format_variant = std::variant<
    types::CppBinary,
    types::CppLibrary,
    types::CppTest,
    types::GenRule,
    types::SystemLib>;

  explicit Rule(const format_variant&, const PackagePath& path);

  ~Rule();

  types::Rule to_format() const;

  PackagePath name() const;

  std::set<std::string> sources() const;
  std::set<std::string> headers() const;
  std::set<std::string> data() const;
  std::set<PackagePath> libs() const;
  std::set<PackagePath> deps() const;
  std::vector<std::string> ld_flags() const;
  std::vector<std::string> cpp_flags() const;
  std::optional<PackagePath> system_lib_config() const;
  std::vector<types::OS> os_filter() const;

  const PackagePath& package_path() const;

  std::optional<PackagePath> output_cpp_object() const;

  const std::optional<yasf::Location>& location() const;

  types::Rule raw() const
  {
    return std::visit([](const auto& r) { return types::Rule(r.raw()); }, rule);
  }

  rule_variant rule;
};

} // namespace mellow::rules
