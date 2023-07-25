#include "generated_build_config.hpp"

#include <type_traits>

#include "bee/format.hpp"
#include "bee/util.hpp"
#include "yasf/parser_helpers.hpp"
#include "yasf/serializer.hpp"
#include "yasf/to_stringable_mixin.hpp"

using PH = yasf::ParserHelper;

namespace generated_build_config {

////////////////////////////////////////////////////////////////////////////////
// Cpp
//

bee::OrError<Cpp> Cpp::of_yasf_value(const yasf::Value::ptr& value)
{
  if (!value->is_list()) {
    return PH::err("Record expected a list, but got something else", value);
  }

  std::optional<std::string> output_compiler;
  std::optional<std::vector<std::string>> output_cpp_flags;
  std::optional<std::vector<std::string>> output_ld_flags;

  for (const auto& element : value->list()) {
    if (!element->is_key_value()) {
      return PH::err("Expected a key value as a record element", element);
    }

    const auto& kv = element->key_value();
    const std::string& name = kv.key;
    if (name == "compiler") {
      if (output_compiler.has_value()) {
        return PH::err("Field 'compiler' is defined more than once", element);
      }
      bail_assign(output_compiler, yasf::des<std::string>(kv.value));
    } else if (name == "cpp_flags") {
      if (output_cpp_flags.has_value()) {
        return PH::err("Field 'cpp_flags' is defined more than once", element);
      }
      bail_assign(
        output_cpp_flags, yasf::des<std::vector<std::string>>(kv.value));
    } else if (name == "ld_flags") {
      if (output_ld_flags.has_value()) {
        return PH::err("Field 'ld_flags' is defined more than once", element);
      }
      bail_assign(
        output_ld_flags, yasf::des<std::vector<std::string>>(kv.value));
    } else {
      return PH::err("No such field in record of type Cpp", element);
    }
  }

  if (!output_compiler.has_value()) {
    return PH::err("Field 'compiler' not defined", value);
  }
  if (!output_cpp_flags.has_value()) { output_cpp_flags.emplace(); }
  if (!output_ld_flags.has_value()) { output_ld_flags.emplace(); }
  return Cpp{
    .compiler = std::move(*output_compiler),
    .cpp_flags = std::move(*output_cpp_flags),
    .ld_flags = std::move(*output_ld_flags),
  };
}

yasf::Value::ptr Cpp::to_yasf_value() const
{
  std::vector<yasf::Value::ptr> fields;
  PH::push_back_field(fields, yasf::ser<std::string>(compiler), "compiler");
  if (!cpp_flags.empty()) {
    PH::push_back_field(
      fields, yasf::ser<std::vector<std::string>>(cpp_flags), "cpp_flags");
  }
  if (!ld_flags.empty()) {
    PH::push_back_field(
      fields, yasf::ser<std::vector<std::string>>(ld_flags), "ld_flags");
  }
  return yasf::Value::create_list(std::move(fields), std::nullopt);
}

////////////////////////////////////////////////////////////////////////////////
// Rule
//

Rule::Rule(const value_type& value) noexcept : value(value) {}
Rule::Rule(value_type&& value) noexcept : value(std::move(value)) {}

bee::OrError<Rule> Rule::of_yasf_value(const yasf::Value::ptr& arg_value)
{
  auto value = arg_value;
  if (value->is_list() && value->list().size() == 1) {
    value = value->list()[0];
  }
  if (!value->is_key_value()) {
    return PH::err("Expected a key value as a variant element", value);
  }

  const auto& kv = value->key_value();
  const std::string& name = kv.key;
  if (name == "cpp") {
    return Cpp::of_yasf_value(kv.value);
  } else {
    return PH::err("Unknown variant leg", value);
  }
}

yasf::Value::ptr Rule::to_yasf_value() const
{
  return visit([](const auto& leg) {
    using T = std::decay_t<decltype(leg)>;
    if constexpr (std::is_same_v<T, Cpp>) {
      return yasf::Value::create_key_value(
        "cpp", {(leg).to_yasf_value()}, std::nullopt);
    }
  });
}

} // namespace generated_build_config

// olint-allow: missing-package-namespace
