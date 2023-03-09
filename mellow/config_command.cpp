#include "config_command.hpp"

#include "build_config.hpp"
#include "generate_build_config.hpp"

#include "command/command_builder.hpp"

namespace mellow {

command::Cmd ConfigCommand::command()
{
  using namespace command;
  using namespace command::flags;
  auto builder = CommandBuilder("Generate a build config");
  auto default_cpp_compiler = builder.optional("--cpp-compiler", string_flag);
  auto output =
    builder.optional_with_default("--output", string_flag, ".build-config");
  return builder.run([=]() {
    return GenerateBuildConfig::generate(
      *output,
      {
        .default_cpp_compiler = *default_cpp_compiler,
      });
  });
}

} // namespace mellow
