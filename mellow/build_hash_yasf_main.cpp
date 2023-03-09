#include "yasf/core_types.hpp"
#include "yasf/generator.hpp"
#include "yasf/generator_main_lib.hpp"

using namespace yasf;

namespace mellow {

Definitions create_def()
{
  using namespace types;
  constexpr auto file_hash = record(
    "FileHash",
    fields(
      required_field("name", str),
      required_field("hash", str),
      required_field("mtime", Time)));

  constexpr auto task_cache = record(
    "TaskHash",
    fields(
      required_field("inputs", vec(file_hash)),
      required_field("outputs", vec(file_hash)),
      required_field("flags_hash", str)));

  return {
    .types =
      {
        file_hash,
        task_cache,
      },
  };
}

} // namespace mellow

Definitions create_def() { return mellow::create_def(); }
