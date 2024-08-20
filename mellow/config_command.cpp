#include "config_command.hpp"

#include "generate_build_config.hpp"

#include "command/command_builder.hpp"
#include "command/file_path.hpp"

namespace mellow {

command::Cmd ConfigCommand::command()
{
  namespace f = command::flags;
  auto builder = command::CommandBuilder("Generate a build config");
  auto default_cpp_compiler = builder.optional("--cpp-compiler", f::FilePath);
  auto output = builder.optional_with_default(
    "--output", f::FilePath, bee::FilePath("build/.build-config"));
  return builder.run([=]() {
    return GenerateBuildConfig::generate(
      *output,
      {
        .default_cpp_compiler = *default_cpp_compiler,
      });
  });
}

} // namespace mellow
