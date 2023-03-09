#include "yasf/generator.hpp"
#include "yasf/generator_main_lib.hpp"

using namespace yasf;

namespace mellow {

Definitions create_def()
{
  using namespace types;
  constexpr auto vector_str = vec(str);

  constexpr auto cpp = record(
    "Cpp",
    fields(
      required_field("compiler", str),
      optional_field("cpp_flags", vector_str),
      optional_field("ld_flags", vector_str)));

  constexpr auto rule_variant = variant("Rule", vlegs(vleg("cpp", cpp)));

  return {
    .types =
      {
        cpp,
        rule_variant,
      },
  };
}

} // namespace mellow

Definitions create_def() { return mellow::create_def(); }
