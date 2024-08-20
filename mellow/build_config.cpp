#include "build_config.hpp"

#include "bee/file_writer.hpp"
#include "yasf/config_parser.hpp"

using std::get;
using std::holds_alternative;
using std::string;
using std::vector;

namespace mellow {

BuildConfig::BuildConfig() {}

BuildConfig::~BuildConfig() {}

yasf::Value::ptr BuildConfig::to_yasf_value() const { return yasf::ser(rules); }

bee::OrError<BuildConfig> BuildConfig::of_yasf_value(
  const yasf::Value::ptr& value)
{
  BuildConfig config;
  bail_assign(config.rules, yasf::des<vector<generated::Rule>>(value));
  return config;
}

generated::Cpp BuildConfig::cpp_config() const
{
  for (const auto& config : rules) {
    if (holds_alternative<generated::Cpp>(config.value)) {
      return get<generated::Cpp>(config.value);
    }
  }
  return {
    .compiler = bee::FilePath("g++"),
  };
}

bee::OrError<BuildConfig> BuildConfig::load_from_file(
  const bee::FilePath& filename)
{
  bail(parsed, yasf::ConfigParser::parse_from_file(filename));
  return of_yasf_value(parsed);
}

bee::OrError<> BuildConfig::write_to_file(const bee::FilePath& filename) const
{
  return bee::FileWriter::write_file(
    filename, to_yasf_value()->to_string_hum());
}

} // namespace mellow
