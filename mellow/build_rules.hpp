#pragma once

#include <variant>

#include "generated_mbuild_parser.hpp"
#include "package_path.hpp"
#include "rule_templates.hpp"

#include "yasf/location.hpp"

namespace mellow {

namespace gmp = generated_mbuild_parser;

namespace rules {

struct CppBinary : public BaseRule<gmp::CppBinary> {
 public:
  explicit CppBinary(const gmp::CppBinary&, const PackagePath&);
  ~CppBinary();
};

namespace details {

struct CppLibraryBase : public gmp::CppLibrary {
  CppLibraryBase(const gmp::CppLibrary&);
  std::optional<std::string> output_cpp_object() const;
};

} // namespace details

struct CppLibrary : public BaseRule<details::CppLibraryBase> {
 public:
  explicit CppLibrary(const gmp::CppLibrary&, const PackagePath&);
  ~CppLibrary();
};

struct CppTest : public BaseRule<gmp::CppTest> {
 public:
  explicit CppTest(const gmp::CppTest&, const PackagePath&);
  ~CppTest();

  const std::vector<std::string>& os_filter() const;
  std::vector<std::string>& os_filter();
};

struct GenRule : public BaseRule<gmp::GenRule> {
 public:
  explicit GenRule(const gmp::GenRule&, const PackagePath&);
  ~GenRule();

  std::set<std::string> additional_deps() const;
};

namespace details {

struct SystemLibBase : public gmp::SystemLib {
  SystemLibBase(const gmp::SystemLib&);
  std::string system_lib_config() const;
};

} // namespace details

struct SystemLib : public BaseRule<details::SystemLibBase> {
 public:
  explicit SystemLib(const gmp::SystemLib&, const PackagePath&);
  ~SystemLib();

  std::vector<std::string> provide_headers() const;
};

struct ExternalPackage : public BaseRule<gmp::ExternalPackage> {
 public:
  explicit ExternalPackage(const gmp::ExternalPackage&);
  ~ExternalPackage();

  std::vector<std::string> provide_headers() const;
};

struct Rule {
  using rule_variant =
    std::variant<CppBinary, CppLibrary, CppTest, GenRule, SystemLib>;

  using format_variant = std::variant<
    gmp::CppBinary,
    gmp::CppLibrary,
    gmp::CppTest,
    gmp::GenRule,
    gmp::SystemLib>;

  explicit Rule(const format_variant&, const PackagePath& path);

  ~Rule();

  gmp::Rule to_format() const;

  PackagePath name() const;

  std::set<std::string> sources() const;
  std::set<std::string> headers() const;
  std::set<PackagePath> libs() const;
  std::set<PackagePath> deps() const;
  std::vector<std::string> ld_flags() const;
  std::vector<std::string> cpp_flags() const;
  std::optional<PackagePath> system_lib_config() const;
  std::vector<std::string> os_filter() const;

  const PackagePath& package_path() const;

  std::optional<PackagePath> output_cpp_object() const;

  const std::optional<yasf::Location>& location() const;

  gmp::Rule raw() const
  {
    return std::visit([](const auto& r) { return gmp::Rule(r.raw()); }, rule);
  }

  rule_variant rule;
};

} // namespace rules

} // namespace mellow
