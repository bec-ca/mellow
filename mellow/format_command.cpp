#include "format_command.hpp"

#include <iostream>

#include "mbuild_parser.hpp"

#include "bee/concepts.hpp"
#include "bee/file_reader.hpp"
#include "bee/file_writer.hpp"
#include "bee/filesystem.hpp"
#include "bee/sort.hpp"
#include "command/command_builder.hpp"

using bee::Error;
using bee::FilePath;
using bee::OrError;
using std::is_same_v;
using std::optional;
using std::string;

namespace mellow {

namespace {

OrError<> run_format(
  bool inplace, optional<string> filename_opt, bool check_only)
{
  if (inplace && check_only) {
    return Error(
      "The modes inplace and check-only can't be enabled at the same time");
  }
  bool has_filename = filename_opt.has_value() && filename_opt != "-";
  auto filename = FilePath(has_filename ? *filename_opt : "");

  if (inplace && !has_filename) {
    return Error("Filename required for inplace mode");
  }

  bail(
    unformatted_content,
    has_filename ? bee::FileReader::read_file(filename)
                 : bee::FileSystem::read_stream(std::cin));

  bail(rules, MbuildParser::from_string(filename, unformatted_content));

  for (auto& rule : rules) {
    std::visit(
      []<class T>(T& rule) {
        if constexpr (is_same_v<T, types::Profile>) {
        } else if constexpr (is_same_v<T, types::SystemLib>) {
        } else if constexpr (is_same_v<T, types::ExternalPackage>) {
        } else if constexpr (is_same_v<T, types::CppBinary>) {
          bee::sort(rule.sources);
          bee::sort(rule.libs);
        } else if constexpr (is_same_v<T, types::CppLibrary>) {
          bee::sort(rule.sources);
          bee::sort(rule.headers);
          bee::sort(rule.libs);
        } else if constexpr (is_same_v<T, types::CppTest>) {
          bee::sort(rule.sources);
          bee::sort(rule.libs);
        } else if constexpr (is_same_v<T, types::GenRule>) {
          bee::sort(rule.outputs);
        } else {
          static_assert(bee::always_false<T> && "non exhaustive visit");
        }
      },
      rule.value);
  }

  auto formated_content = MbuildParser::to_string(rules);

  if (check_only) {
    if (formated_content != unformatted_content) {
      return Error("Format diff found");
    }
  } else if (inplace) {
    bail_unit(bee::FileWriter::write_file(filename, formated_content));
  } else {
    std::cout << formated_content;
  }

  return bee::ok();
}

} // namespace

using command::Cmd;
using command::CommandBuilder;

Cmd FormatCommand::command()
{
  namespace f = command::flags;
  auto builder = CommandBuilder("Format mbuild file");
  auto inplace = builder.no_arg("--inplace");
  auto check_only = builder.no_arg("--check-only");
  auto filename = builder.anon(f::String, "mbuild-file");
  return builder.run(
    [=]() { return run_format(*inplace, *filename, *check_only); });
}

} // namespace mellow
