#pragma once

#include "bee/error.hpp"
#include "yasf/serializer.hpp"
#include "yasf/to_stringable_mixin.hpp"

#include <set>
#include <string>
#include <variant>
#include <vector>

namespace generated_mbuild_parser {

struct Profile : public yasf::ToStringableMixin<Profile> {
  std::string name;
  std::vector<std::string> cpp_flags;
  std::vector<std::string> ld_flags = {};
  std::optional<std::string> cpp_compiler = std::nullopt;
  std::optional<yasf::Location> location;

  static bee::OrError<Profile> of_yasf_value(
    const yasf::Value::ptr& config_value);

  yasf::Value::ptr to_yasf_value() const;
};

struct CppBinary : public yasf::ToStringableMixin<CppBinary> {
  std::string name;
  std::vector<std::string> sources = {};
  std::vector<std::string> libs;
  std::vector<std::string> ld_flags = {};
  std::vector<std::string> cpp_flags = {};
  std::optional<yasf::Location> location;

  static bee::OrError<CppBinary> of_yasf_value(
    const yasf::Value::ptr& config_value);

  yasf::Value::ptr to_yasf_value() const;
};

struct CppLibrary : public yasf::ToStringableMixin<CppLibrary> {
  std::string name;
  std::vector<std::string> sources = {};
  std::vector<std::string> headers = {};
  std::vector<std::string> libs = {};
  std::vector<std::string> ld_flags = {};
  std::vector<std::string> cpp_flags = {};
  std::optional<yasf::Location> location;

  static bee::OrError<CppLibrary> of_yasf_value(
    const yasf::Value::ptr& config_value);

  yasf::Value::ptr to_yasf_value() const;
};

struct CppTest : public yasf::ToStringableMixin<CppTest> {
  std::string name;
  std::vector<std::string> sources;
  std::vector<std::string> libs = {};
  std::string output;
  std::vector<std::string> os_filter = {};
  std::optional<yasf::Location> location;

  static bee::OrError<CppTest> of_yasf_value(
    const yasf::Value::ptr& config_value);

  yasf::Value::ptr to_yasf_value() const;
};

struct GenRule : public yasf::ToStringableMixin<GenRule> {
  std::string name;
  std::string binary;
  std::vector<std::string> flags = {};
  std::vector<std::string> outputs;
  std::optional<yasf::Location> location;

  static bee::OrError<GenRule> of_yasf_value(
    const yasf::Value::ptr& config_value);

  yasf::Value::ptr to_yasf_value() const;
};

struct SystemLib : public yasf::ToStringableMixin<SystemLib> {
  std::string name;
  std::string command;
  std::vector<std::string> flags = {};
  std::vector<std::string> provide_headers;
  std::optional<yasf::Location> location;

  static bee::OrError<SystemLib> of_yasf_value(
    const yasf::Value::ptr& config_value);

  yasf::Value::ptr to_yasf_value() const;
};

struct ExternalPackage : public yasf::ToStringableMixin<ExternalPackage> {
  std::string name;
  std::optional<std::string> source = std::nullopt;
  std::optional<std::string> url = std::nullopt;
  std::optional<yasf::Location> location;

  static bee::OrError<ExternalPackage> of_yasf_value(
    const yasf::Value::ptr& config_value);

  yasf::Value::ptr to_yasf_value() const;
};

struct Rule {
  using value_type = std::variant<
    Profile,
    CppBinary,
    CppLibrary,
    CppTest,
    GenRule,
    SystemLib,
    ExternalPackage>;

  value_type value;

  Rule() noexcept = default;
  Rule(const value_type& value) noexcept;
  Rule(value_type&& value) noexcept;

  static bee::OrError<Rule> of_yasf_value(const yasf::Value::ptr& config_value);

  template <class F> auto visit(F&& f) const
  {
    return std::visit(std::forward<F>(f), value);
  }
  template <class F> auto visit(F&& f)
  {
    return std::visit(std::forward<F>(f), value);
  }
  yasf::Value::ptr to_yasf_value() const;
};

} // namespace generated_mbuild_parser

// olint-allow: missing-package-namespace
