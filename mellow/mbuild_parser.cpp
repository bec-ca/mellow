#include "mbuild_parser.hpp"

#include "bee/file_reader.hpp"
#include "bee/file_writer.hpp"
#include "build_rules.hpp"
#include "yasf/config_parser.hpp"

#include <string>
#include <vector>

using bee::FilePath;
using bee::OrError;
using bee::Unit;
using std::string;
using std::vector;

namespace mellow {

using Rules = MbuildParser::Rules;

OrError<Rules> MbuildParser::from_string(
  const FilePath& filename, const string& content)
{
  bail(
    parsed,
    yasf::ConfigParser::parse_from_string(filename.to_std_path(), content));
  return yasf::des<vector<gmp::Rule>>(parsed);
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

OrError<Unit> MbuildParser::to_file(
  const FilePath& filename, const Rules& config)
{
  return bee::FileWriter::save_file(filename, to_string(config));
}

} // namespace mellow
