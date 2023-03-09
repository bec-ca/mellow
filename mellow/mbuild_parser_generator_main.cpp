#include "yasf/generator.hpp"
#include "yasf/generator_main_lib.hpp"
#include "yasf/record.hpp"

using namespace yasf;

namespace mellow {

Definitions create_def()
{
  using namespace types;
  constexpr auto vector_str = vec(str);

  constexpr auto profile = record_with_location(
    "Profile",
    fields(
      required_field("name", str),
      required_field("cpp_flags", vector_str),
      optional_field("ld_flags", vector_str),
      optional_field("cpp_compiler", str)));

  constexpr auto cpp_binary = record_with_location(
    "CppBinary",
    fields(
      required_field("name", str),
      optional_field("sources", vector_str),
      required_field("libs", vector_str),
      optional_field("ld_flags", vector_str),
      optional_field("cpp_flags", vector_str)));

  constexpr auto cpp_library = record_with_location(
    "CppLibrary",
    fields(
      required_field("name", str),
      optional_field("sources", vector_str),
      optional_field("headers", vector_str),
      optional_field("libs", vector_str),
      optional_field("ld_flags", vector_str),
      optional_field("cpp_flags", vector_str)));

  constexpr auto cpp_test = record_with_location(
    "CppTest",
    fields(
      required_field("name", str),
      required_field("sources", vector_str),
      optional_field("libs", vector_str),
      required_field("output", str),
      optional_field("os_filter", vector_str)));

  constexpr auto gen_rule = record_with_location(
    "GenRule",
    fields(
      required_field("name", str),
      required_field("binary", str),
      optional_field("flags", vector_str),
      required_field("outputs", vector_str)));

  constexpr auto system_lib = record_with_location(
    "SystemLib",
    fields(
      required_field("name", str),
      required_field("command", str),
      optional_field("flags", vector_str),
      required_field("provide_headers", vector_str)));

  constexpr auto external_package = record_with_location(
    "ExternalPackage",
    fields(
      required_field("name", str),
      optional_field("source", str),
      optional_field("url", str)));

  constexpr auto rule_variant = variant(
    "Rule",
    vlegs(
      vleg("profile", profile),
      vleg("cpp_binary", cpp_binary),
      vleg("cpp_library", cpp_library),
      vleg("cpp_test", cpp_test),
      vleg("gen_rule", gen_rule),
      vleg("system_lib", system_lib),
      vleg("external_package", external_package)));

  return Definitions{
    .types =
      {
        profile,
        cpp_binary,
        cpp_library,
        cpp_test,
        gen_rule,
        system_lib,
        external_package,
        rule_variant,
      },
  };
}

} // namespace mellow

Definitions create_def() { return mellow::create_def(); }
