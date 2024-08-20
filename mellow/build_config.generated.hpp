#pragma once

#include <set>
#include <string>
#include <variant>
#include <vector>

#include "bee/or_error.hpp"
#include "yasf/file_path.hpp"
#include "yasf/serializer.hpp"
#include "yasf/to_stringable_mixin.hpp"

namespace mellow::generated {

struct Cpp : public yasf::ToStringableMixin<Cpp> {
  yasf::FilePath compiler;
  std::vector<std::string> cpp_flags{};
  std::vector<std::string> ld_flags{};

  static bee::OrError<Cpp> of_yasf_value(const yasf::Value::ptr& config_value);

  yasf::Value::ptr to_yasf_value() const;
};

struct Rule : public yasf::ToStringableMixin<Rule> {
  using value_type = std::variant<Cpp>;

  value_type value;

  Rule() noexcept = default;
  Rule(const value_type& value) noexcept;
  Rule(value_type&& value) noexcept;

  template <std::convertible_to<value_type> U>
  Rule(U&& value) noexcept : value(std::forward<U>(value))
  {}

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

} // namespace mellow::generated
