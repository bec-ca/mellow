#include "build_command.hpp"
#include "config_command.hpp"
#include "fetch_command.hpp"
#include "format_command.hpp"
#include "genbuild_command.hpp"

#include "command/group_builder.hpp"

namespace mellow {
namespace {

int main(int argc, char* argv[])
{
  return command::GroupBuilder("Mellow")
    .cmd("format", FormatCommand::command())
    .cmd("build", BuildCommand::command())
    .cmd("genbuild", GenbuildCommand::command())
    .cmd("fetch", FetchCommand::command())
    .cmd("config", ConfigCommand::command())
    .build()
    .main(argc, argv);
}

} // namespace
} // namespace mellow

int main(int argc, char* argv[]) { return mellow::main(argc, argv); }
