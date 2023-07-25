#include "build_config.hpp"

#include <filesystem>

#include "bee/file_reader.hpp"
#include "bee/file_writer.hpp"
#include "bee/string_util.hpp"
#include "bee/util.hpp"
#include "yasf/config_parser.hpp"

namespace fs = std::filesystem;

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
  bail_assign(config.rules, yasf::des<vector<bc::Rule>>(value));
  return config;
}

bc::Cpp BuildConfig::cpp_config() const
{
  for (const auto& config : rules) {
    if (holds_alternative<bc::Cpp>(config.value)) {
      return get<bc::Cpp>(config.value);
    }
  }
  return bc::Cpp{
    .compiler = "g++",
  };
}

bee::OrError<BuildConfig> BuildConfig::load_from_file(const fs::path& filename)
{
  bail(parsed, yasf::ConfigParser::parse_from_file(filename));
  return of_yasf_value(parsed);
}

bee::OrError<> BuildConfig::write_to_file(const fs::path& filename) const
{
  return bee::FileWriter::save_file(
    bee::FilePath::of_std_path(filename), to_yasf_value()->to_string_hum());
}

} // namespace mellow
