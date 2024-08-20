#include "mbuild_parser.hpp"

#include <string>
#include <vector>

#include "build_rules.hpp"

#include "bee/file_reader.hpp"
#include "bee/file_writer.hpp"
#include "yasf/config_parser.hpp"

using bee::FilePath;
using bee::OrError;
using std::string;
using std::vector;

namespace mellow {

using Rules = MbuildParser::Rules;

OrError<Rules> MbuildParser::from_string(
  const FilePath& filename, const string& content)
{
  bail(parsed, yasf::ConfigParser::parse_from_string(filename, content));
  return yasf::des<vector<types::Rule>>(parsed);
}

OrError<Rules> MbuildParser::from_file(const FilePath& filename)
{
  bail(content, bee::FileReader::read_file(filename));
  return from_string(filename, content);
}

string MbuildParser::to_string(const Rules& config)
{
  return yasf::ser(config)->to_string_hum();
}

OrError<> MbuildParser::to_file(const FilePath& filename, const Rules& config)
{
  return bee::FileWriter::write_file(filename, to_string(config));
}

} // namespace mellow
