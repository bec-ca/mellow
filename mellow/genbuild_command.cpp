#include "genbuild_command.hpp"

#include "genbuild.hpp"

#include "command/command_builder.hpp"

using command::Cmd;
using command::CommandBuilder;

namespace mellow {

Cmd GenbuildCommand::command()
{
  using namespace command::flags;
  auto builder = CommandBuilder("Generate an mbuild file");
  auto directory = builder.anon(string_flag, "dir");
  auto output = builder.optional("--output-mbuild", string_flag);
  return builder.run([=]() { return GenBuild::run(*directory, *output); });
}

} // namespace mellow
