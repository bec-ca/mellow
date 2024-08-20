#include "genbuild_command.hpp"

#include "genbuild.hpp"

#include "command/command_builder.hpp"
#include "command/file_path.hpp"

using command::Cmd;
using command::CommandBuilder;

namespace mellow {

Cmd GenbuildCommand::command()
{
  namespace f = command::flags;
  auto builder = CommandBuilder("Generate an mbuild file");
  auto directory = builder.anon(f::FilePath, "dir");
  auto output = builder.optional("--output-mbuild", f::FilePath);
  return builder.run([=]() { return GenBuild::run(*directory, *output); });
}

} // namespace mellow
