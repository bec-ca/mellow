#include "generated_mbuild_parser.hpp"

#include <type_traits>

#include "bee/format.hpp"
#include "bee/util.hpp"
#include "yasf/file_path_serializer.hpp"
#include "yasf/parser_helpers.hpp"
#include "yasf/serializer.hpp"
#include "yasf/to_stringable_mixin.hpp"

using PH = yasf::ParserHelper;

namespace generated_mbuild_parser {

////////////////////////////////////////////////////////////////////////////////
// Profile
//

bee::OrError<Profile> Profile::of_yasf_value(const yasf::Value::ptr& value)
{
  if (!value->is_list()) {
    return PH::err("Record expected a list, but got something else", value);
  }

  std::optional<std::string> output_name;
  std::optional<std::vector<std::string>> output_cpp_flags;
  std::optional<std::vector<std::string>> output_ld_flags;
  std::optional<std::string> output_cpp_compiler;

  for (const auto& element : value->list()) {
    if (!element->is_key_value()) {
      return PH::err("Expected a key value as a record element", element);
    }

    const auto& kv = element->key_value();
    const std::string& name = kv.key;
    if (name == "name") {
      if (output_name.has_value()) {
        return PH::err("Field 'name' is defined more than once", element);
      }
      bail_assign(output_name, yasf::des<std::string>(kv.value));
    } else if (name == "cpp_flags") {
      if (output_cpp_flags.has_value()) {
        return PH::err("Field 'cpp_flags' is defined more than once", element);
      }
      bail_assign(
        output_cpp_flags, yasf::des<std::vector<std::string>>(kv.value));
    } else if (name == "ld_flags") {
      if (output_ld_flags.has_value()) {
        return PH::err("Field 'ld_flags' is defined more than once", element);
      }
      bail_assign(
        output_ld_flags, yasf::des<std::vector<std::string>>(kv.value));
    } else if (name == "cpp_compiler") {
      if (output_cpp_compiler.has_value()) {
        return PH::err(
          "Field 'cpp_compiler' is defined more than once", element);
      }
      bail_assign(output_cpp_compiler, yasf::des<std::string>(kv.value));
    } else {
      return PH::err("No such field in record of type Profile", element);
    }
  }

  if (!output_name.has_value()) {
    return PH::err("Field 'name' not defined", value);
  }
  if (!output_cpp_flags.has_value()) {
    return PH::err("Field 'cpp_flags' not defined", value);
  }
  if (!output_ld_flags.has_value()) { output_ld_flags.emplace(); }
  return Profile{
    .name = std::move(*output_name),
    .cpp_flags = std::move(*output_cpp_flags),
    .ld_flags = std::move(*output_ld_flags),
    .cpp_compiler = std::move(output_cpp_compiler),
    .location = value->location(),
  };
}

yasf::Value::ptr Profile::to_yasf_value() const
{
  std::vector<yasf::Value::ptr> fields;
  PH::push_back_field(fields, yasf::ser<std::string>(name), "name");
  PH::push_back_field(
    fields, yasf::ser<std::vector<std::string>>(cpp_flags), "cpp_flags");
  if (!ld_flags.empty()) {
    PH::push_back_field(
      fields, yasf::ser<std::vector<std::string>>(ld_flags), "ld_flags");
  }
  if (cpp_compiler.has_value()) {
    PH::push_back_field(
      fields, yasf::ser<std::string>(*cpp_compiler), "cpp_compiler");
  }
  return yasf::Value::create_list(std::move(fields), std::nullopt);
}

////////////////////////////////////////////////////////////////////////////////
// CppBinary
//

bee::OrError<CppBinary> CppBinary::of_yasf_value(const yasf::Value::ptr& value)
{
  if (!value->is_list()) {
    return PH::err("Record expected a list, but got something else", value);
  }

  std::optional<std::string> output_name;
  std::optional<std::vector<std::string>> output_sources;
  std::optional<std::vector<std::string>> output_libs;
  std::optional<std::vector<std::string>> output_ld_flags;
  std::optional<std::vector<std::string>> output_cpp_flags;

  for (const auto& element : value->list()) {
    if (!element->is_key_value()) {
      return PH::err("Expected a key value as a record element", element);
    }

    const auto& kv = element->key_value();
    const std::string& name = kv.key;
    if (name == "name") {
      if (output_name.has_value()) {
        return PH::err("Field 'name' is defined more than once", element);
      }
      bail_assign(output_name, yasf::des<std::string>(kv.value));
    } else if (name == "sources") {
      if (output_sources.has_value()) {
        return PH::err("Field 'sources' is defined more than once", element);
      }
      bail_assign(
        output_sources, yasf::des<std::vector<std::string>>(kv.value));
    } else if (name == "libs") {
      if (output_libs.has_value()) {
        return PH::err("Field 'libs' is defined more than once", element);
      }
      bail_assign(output_libs, yasf::des<std::vector<std::string>>(kv.value));
    } else if (name == "ld_flags") {
      if (output_ld_flags.has_value()) {
        return PH::err("Field 'ld_flags' is defined more than once", element);
      }
      bail_assign(
        output_ld_flags, yasf::des<std::vector<std::string>>(kv.value));
    } else if (name == "cpp_flags") {
      if (output_cpp_flags.has_value()) {
        return PH::err("Field 'cpp_flags' is defined more than once", element);
      }
      bail_assign(
        output_cpp_flags, yasf::des<std::vector<std::string>>(kv.value));
    } else {
      return PH::err("No such field in record of type CppBinary", element);
    }
  }

  if (!output_name.has_value()) {
    return PH::err("Field 'name' not defined", value);
  }
  if (!output_sources.has_value()) { output_sources.emplace(); }
  if (!output_libs.has_value()) {
    return PH::err("Field 'libs' not defined", value);
  }
  if (!output_ld_flags.has_value()) { output_ld_flags.emplace(); }
  if (!output_cpp_flags.has_value()) { output_cpp_flags.emplace(); }
  return CppBinary{
    .name = std::move(*output_name),
    .sources = std::move(*output_sources),
    .libs = std::move(*output_libs),
    .ld_flags = std::move(*output_ld_flags),
    .cpp_flags = std::move(*output_cpp_flags),
    .location = value->location(),
  };
}

yasf::Value::ptr CppBinary::to_yasf_value() const
{
  std::vector<yasf::Value::ptr> fields;
  PH::push_back_field(fields, yasf::ser<std::string>(name), "name");
  if (!sources.empty()) {
    PH::push_back_field(
      fields, yasf::ser<std::vector<std::string>>(sources), "sources");
  }
  PH::push_back_field(
    fields, yasf::ser<std::vector<std::string>>(libs), "libs");
  if (!ld_flags.empty()) {
    PH::push_back_field(
      fields, yasf::ser<std::vector<std::string>>(ld_flags), "ld_flags");
  }
  if (!cpp_flags.empty()) {
    PH::push_back_field(
      fields, yasf::ser<std::vector<std::string>>(cpp_flags), "cpp_flags");
  }
  return yasf::Value::create_list(std::move(fields), std::nullopt);
}

////////////////////////////////////////////////////////////////////////////////
// CppLibrary
//

bee::OrError<CppLibrary> CppLibrary::of_yasf_value(
  const yasf::Value::ptr& value)
{
  if (!value->is_list()) {
    return PH::err("Record expected a list, but got something else", value);
  }

  std::optional<std::string> output_name;
  std::optional<std::vector<std::string>> output_sources;
  std::optional<std::vector<std::string>> output_headers;
  std::optional<std::vector<std::string>> output_libs;
  std::optional<std::vector<std::string>> output_ld_flags;
  std::optional<std::vector<std::string>> output_cpp_flags;

  for (const auto& element : value->list()) {
    if (!element->is_key_value()) {
      return PH::err("Expected a key value as a record element", element);
    }

    const auto& kv = element->key_value();
    const std::string& name = kv.key;
    if (name == "name") {
      if (output_name.has_value()) {
        return PH::err("Field 'name' is defined more than once", element);
      }
      bail_assign(output_name, yasf::des<std::string>(kv.value));
    } else if (name == "sources") {
      if (output_sources.has_value()) {
        return PH::err("Field 'sources' is defined more than once", element);
      }
      bail_assign(
        output_sources, yasf::des<std::vector<std::string>>(kv.value));
    } else if (name == "headers") {
      if (output_headers.has_value()) {
        return PH::err("Field 'headers' is defined more than once", element);
      }
      bail_assign(
        output_headers, yasf::des<std::vector<std::string>>(kv.value));
    } else if (name == "libs") {
      if (output_libs.has_value()) {
        return PH::err("Field 'libs' is defined more than once", element);
      }
      bail_assign(output_libs, yasf::des<std::vector<std::string>>(kv.value));
    } else if (name == "ld_flags") {
      if (output_ld_flags.has_value()) {
        return PH::err("Field 'ld_flags' is defined more than once", element);
      }
      bail_assign(
        output_ld_flags, yasf::des<std::vector<std::string>>(kv.value));
    } else if (name == "cpp_flags") {
      if (output_cpp_flags.has_value()) {
        return PH::err("Field 'cpp_flags' is defined more than once", element);
      }
      bail_assign(
        output_cpp_flags, yasf::des<std::vector<std::string>>(kv.value));
    } else {
      return PH::err("No such field in record of type CppLibrary", element);
    }
  }

  if (!output_name.has_value()) {
    return PH::err("Field 'name' not defined", value);
  }
  if (!output_sources.has_value()) { output_sources.emplace(); }
  if (!output_headers.has_value()) { output_headers.emplace(); }
  if (!output_libs.has_value()) { output_libs.emplace(); }
  if (!output_ld_flags.has_value()) { output_ld_flags.emplace(); }
  if (!output_cpp_flags.has_value()) { output_cpp_flags.emplace(); }
  return CppLibrary{
    .name = std::move(*output_name),
    .sources = std::move(*output_sources),
    .headers = std::move(*output_headers),
    .libs = std::move(*output_libs),
    .ld_flags = std::move(*output_ld_flags),
    .cpp_flags = std::move(*output_cpp_flags),
    .location = value->location(),
  };
}

yasf::Value::ptr CppLibrary::to_yasf_value() const
{
  std::vector<yasf::Value::ptr> fields;
  PH::push_back_field(fields, yasf::ser<std::string>(name), "name");
  if (!sources.empty()) {
    PH::push_back_field(
      fields, yasf::ser<std::vector<std::string>>(sources), "sources");
  }
  if (!headers.empty()) {
    PH::push_back_field(
      fields, yasf::ser<std::vector<std::string>>(headers), "headers");
  }
  if (!libs.empty()) {
    PH::push_back_field(
      fields, yasf::ser<std::vector<std::string>>(libs), "libs");
  }
  if (!ld_flags.empty()) {
    PH::push_back_field(
      fields, yasf::ser<std::vector<std::string>>(ld_flags), "ld_flags");
  }
  if (!cpp_flags.empty()) {
    PH::push_back_field(
      fields, yasf::ser<std::vector<std::string>>(cpp_flags), "cpp_flags");
  }
  return yasf::Value::create_list(std::move(fields), std::nullopt);
}

////////////////////////////////////////////////////////////////////////////////
// CppTest
//

bee::OrError<CppTest> CppTest::of_yasf_value(const yasf::Value::ptr& value)
{
  if (!value->is_list()) {
    return PH::err("Record expected a list, but got something else", value);
  }

  std::optional<std::string> output_name;
  std::optional<std::vector<std::string>> output_sources;
  std::optional<std::vector<std::string>> output_libs;
  std::optional<std::string> output_output;
  std::optional<std::vector<std::string>> output_os_filter;

  for (const auto& element : value->list()) {
    if (!element->is_key_value()) {
      return PH::err("Expected a key value as a record element", element);
    }

    const auto& kv = element->key_value();
    const std::string& name = kv.key;
    if (name == "name") {
      if (output_name.has_value()) {
        return PH::err("Field 'name' is defined more than once", element);
      }
      bail_assign(output_name, yasf::des<std::string>(kv.value));
    } else if (name == "sources") {
      if (output_sources.has_value()) {
        return PH::err("Field 'sources' is defined more than once", element);
      }
      bail_assign(
        output_sources, yasf::des<std::vector<std::string>>(kv.value));
    } else if (name == "libs") {
      if (output_libs.has_value()) {
        return PH::err("Field 'libs' is defined more than once", element);
      }
      bail_assign(output_libs, yasf::des<std::vector<std::string>>(kv.value));
    } else if (name == "output") {
      if (output_output.has_value()) {
        return PH::err("Field 'output' is defined more than once", element);
      }
      bail_assign(output_output, yasf::des<std::string>(kv.value));
    } else if (name == "os_filter") {
      if (output_os_filter.has_value()) {
        return PH::err("Field 'os_filter' is defined more than once", element);
      }
      bail_assign(
        output_os_filter, yasf::des<std::vector<std::string>>(kv.value));
    } else {
      return PH::err("No such field in record of type CppTest", element);
    }
  }

  if (!output_name.has_value()) {
    return PH::err("Field 'name' not defined", value);
  }
  if (!output_sources.has_value()) {
    return PH::err("Field 'sources' not defined", value);
  }
  if (!output_libs.has_value()) { output_libs.emplace(); }
  if (!output_output.has_value()) {
    return PH::err("Field 'output' not defined", value);
  }
  if (!output_os_filter.has_value()) { output_os_filter.emplace(); }
  return CppTest{
    .name = std::move(*output_name),
    .sources = std::move(*output_sources),
    .libs = std::move(*output_libs),
    .output = std::move(*output_output),
    .os_filter = std::move(*output_os_filter),
    .location = value->location(),
  };
}

yasf::Value::ptr CppTest::to_yasf_value() const
{
  std::vector<yasf::Value::ptr> fields;
  PH::push_back_field(fields, yasf::ser<std::string>(name), "name");
  PH::push_back_field(
    fields, yasf::ser<std::vector<std::string>>(sources), "sources");
  if (!libs.empty()) {
    PH::push_back_field(
      fields, yasf::ser<std::vector<std::string>>(libs), "libs");
  }
  PH::push_back_field(fields, yasf::ser<std::string>(output), "output");
  if (!os_filter.empty()) {
    PH::push_back_field(
      fields, yasf::ser<std::vector<std::string>>(os_filter), "os_filter");
  }
  return yasf::Value::create_list(std::move(fields), std::nullopt);
}

////////////////////////////////////////////////////////////////////////////////
// GenRule
//

bee::OrError<GenRule> GenRule::of_yasf_value(const yasf::Value::ptr& value)
{
  if (!value->is_list()) {
    return PH::err("Record expected a list, but got something else", value);
  }

  std::optional<std::string> output_name;
  std::optional<std::string> output_binary;
  std::optional<std::vector<std::string>> output_flags;
  std::optional<std::vector<std::string>> output_outputs;

  for (const auto& element : value->list()) {
    if (!element->is_key_value()) {
      return PH::err("Expected a key value as a record element", element);
    }

    const auto& kv = element->key_value();
    const std::string& name = kv.key;
    if (name == "name") {
      if (output_name.has_value()) {
        return PH::err("Field 'name' is defined more than once", element);
      }
      bail_assign(output_name, yasf::des<std::string>(kv.value));
    } else if (name == "binary") {
      if (output_binary.has_value()) {
        return PH::err("Field 'binary' is defined more than once", element);
      }
      bail_assign(output_binary, yasf::des<std::string>(kv.value));
    } else if (name == "flags") {
      if (output_flags.has_value()) {
        return PH::err("Field 'flags' is defined more than once", element);
      }
      bail_assign(output_flags, yasf::des<std::vector<std::string>>(kv.value));
    } else if (name == "outputs") {
      if (output_outputs.has_value()) {
        return PH::err("Field 'outputs' is defined more than once", element);
      }
      bail_assign(
        output_outputs, yasf::des<std::vector<std::string>>(kv.value));
    } else {
      return PH::err("No such field in record of type GenRule", element);
    }
  }

  if (!output_name.has_value()) {
    return PH::err("Field 'name' not defined", value);
  }
  if (!output_binary.has_value()) {
    return PH::err("Field 'binary' not defined", value);
  }
  if (!output_flags.has_value()) { output_flags.emplace(); }
  if (!output_outputs.has_value()) {
    return PH::err("Field 'outputs' not defined", value);
  }

  return GenRule{
    .name = std::move(*output_name),
    .binary = std::move(*output_binary),
    .flags = std::move(*output_flags),
    .outputs = std::move(*output_outputs),
    .location = value->location(),
  };
}

yasf::Value::ptr GenRule::to_yasf_value() const
{
  std::vector<yasf::Value::ptr> fields;
  PH::push_back_field(fields, yasf::ser<std::string>(name), "name");
  PH::push_back_field(fields, yasf::ser<std::string>(binary), "binary");
  if (!flags.empty()) {
    PH::push_back_field(
      fields, yasf::ser<std::vector<std::string>>(flags), "flags");
  }
  PH::push_back_field(
    fields, yasf::ser<std::vector<std::string>>(outputs), "outputs");
  return yasf::Value::create_list(std::move(fields), std::nullopt);
}

////////////////////////////////////////////////////////////////////////////////
// SystemLib
//

bee::OrError<SystemLib> SystemLib::of_yasf_value(const yasf::Value::ptr& value)
{
  if (!value->is_list()) {
    return PH::err("Record expected a list, but got something else", value);
  }

  std::optional<std::string> output_name;
  std::optional<bee::FilePath> output_command;
  std::optional<std::vector<std::string>> output_flags;
  std::optional<std::vector<std::string>> output_provide_headers;

  for (const auto& element : value->list()) {
    if (!element->is_key_value()) {
      return PH::err("Expected a key value as a record element", element);
    }

    const auto& kv = element->key_value();
    const std::string& name = kv.key;
    if (name == "name") {
      if (output_name.has_value()) {
        return PH::err("Field 'name' is defined more than once", element);
      }
      bail_assign(output_name, yasf::des<std::string>(kv.value));
    } else if (name == "command") {
      if (output_command.has_value()) {
        return PH::err("Field 'command' is defined more than once", element);
      }
      bail_assign(output_command, yasf::des<bee::FilePath>(kv.value));
    } else if (name == "flags") {
      if (output_flags.has_value()) {
        return PH::err("Field 'flags' is defined more than once", element);
      }
      bail_assign(output_flags, yasf::des<std::vector<std::string>>(kv.value));
    } else if (name == "provide_headers") {
      if (output_provide_headers.has_value()) {
        return PH::err(
          "Field 'provide_headers' is defined more than once", element);
      }
      bail_assign(
        output_provide_headers, yasf::des<std::vector<std::string>>(kv.value));
    } else {
      return PH::err("No such field in record of type SystemLib", element);
    }
  }

  if (!output_name.has_value()) {
    return PH::err("Field 'name' not defined", value);
  }
  if (!output_command.has_value()) {
    return PH::err("Field 'command' not defined", value);
  }
  if (!output_flags.has_value()) { output_flags.emplace(); }
  if (!output_provide_headers.has_value()) {
    return PH::err("Field 'provide_headers' not defined", value);
  }

  return SystemLib{
    .name = std::move(*output_name),
    .command = std::move(*output_command),
    .flags = std::move(*output_flags),
    .provide_headers = std::move(*output_provide_headers),
    .location = value->location(),
  };
}

yasf::Value::ptr SystemLib::to_yasf_value() const
{
  std::vector<yasf::Value::ptr> fields;
  PH::push_back_field(fields, yasf::ser<std::string>(name), "name");
  PH::push_back_field(fields, yasf::ser<bee::FilePath>(command), "command");
  if (!flags.empty()) {
    PH::push_back_field(
      fields, yasf::ser<std::vector<std::string>>(flags), "flags");
  }
  PH::push_back_field(
    fields,
    yasf::ser<std::vector<std::string>>(provide_headers),
    "provide_headers");
  return yasf::Value::create_list(std::move(fields), std::nullopt);
}

////////////////////////////////////////////////////////////////////////////////
// ExternalPackage
//

bee::OrError<ExternalPackage> ExternalPackage::of_yasf_value(
  const yasf::Value::ptr& value)
{
  if (!value->is_list()) {
    return PH::err("Record expected a list, but got something else", value);
  }

  std::optional<std::string> output_name;
  std::optional<std::string> output_source;
  std::optional<std::string> output_url;

  for (const auto& element : value->list()) {
    if (!element->is_key_value()) {
      return PH::err("Expected a key value as a record element", element);
    }

    const auto& kv = element->key_value();
    const std::string& name = kv.key;
    if (name == "name") {
      if (output_name.has_value()) {
        return PH::err("Field 'name' is defined more than once", element);
      }
      bail_assign(output_name, yasf::des<std::string>(kv.value));
    } else if (name == "source") {
      if (output_source.has_value()) {
        return PH::err("Field 'source' is defined more than once", element);
      }
      bail_assign(output_source, yasf::des<std::string>(kv.value));
    } else if (name == "url") {
      if (output_url.has_value()) {
        return PH::err("Field 'url' is defined more than once", element);
      }
      bail_assign(output_url, yasf::des<std::string>(kv.value));
    } else {
      return PH::err(
        "No such field in record of type ExternalPackage", element);
    }
  }

  if (!output_name.has_value()) {
    return PH::err("Field 'name' not defined", value);
  }

  return ExternalPackage{
    .name = std::move(*output_name),
    .source = std::move(output_source),
    .url = std::move(output_url),
    .location = value->location(),
  };
}

yasf::Value::ptr ExternalPackage::to_yasf_value() const
{
  std::vector<yasf::Value::ptr> fields;
  PH::push_back_field(fields, yasf::ser<std::string>(name), "name");
  if (source.has_value()) {
    PH::push_back_field(fields, yasf::ser<std::string>(*source), "source");
  }
  if (url.has_value()) {
    PH::push_back_field(fields, yasf::ser<std::string>(*url), "url");
  }
  return yasf::Value::create_list(std::move(fields), std::nullopt);
}

////////////////////////////////////////////////////////////////////////////////
// Rule
//

Rule::Rule(const value_type& value) noexcept : value(value) {}
Rule::Rule(value_type&& value) noexcept : value(std::move(value)) {}

bee::OrError<Rule> Rule::of_yasf_value(const yasf::Value::ptr& arg_value)
{
  auto value = arg_value;
  if (value->is_list() && value->list().size() == 1) {
    value = value->list()[0];
  }
  if (!value->is_key_value()) {
    return PH::err("Expected a key value as a variant element", value);
  }

  const auto& kv = value->key_value();
  const std::string& name = kv.key;
  if (name == "profile") {
    return Profile::of_yasf_value(kv.value);
  } else if (name == "cpp_binary") {
    return CppBinary::of_yasf_value(kv.value);
  } else if (name == "cpp_library") {
    return CppLibrary::of_yasf_value(kv.value);
  } else if (name == "cpp_test") {
    return CppTest::of_yasf_value(kv.value);
  } else if (name == "gen_rule") {
    return GenRule::of_yasf_value(kv.value);
  } else if (name == "system_lib") {
    return SystemLib::of_yasf_value(kv.value);
  } else if (name == "external_package") {
    return ExternalPackage::of_yasf_value(kv.value);
  } else {
    return PH::err("Unknown variant leg", value);
  }
}

yasf::Value::ptr Rule::to_yasf_value() const
{
  return visit([](const auto& leg) {
    using T = std::decay_t<decltype(leg)>;
    if constexpr (std::is_same_v<T, Profile>) {
      return yasf::Value::create_key_value(
        "profile", {(leg).to_yasf_value()}, std::nullopt);
    } else if constexpr (std::is_same_v<T, CppBinary>) {
      return yasf::Value::create_key_value(
        "cpp_binary", {(leg).to_yasf_value()}, std::nullopt);
    } else if constexpr (std::is_same_v<T, CppLibrary>) {
      return yasf::Value::create_key_value(
        "cpp_library", {(leg).to_yasf_value()}, std::nullopt);
    } else if constexpr (std::is_same_v<T, CppTest>) {
      return yasf::Value::create_key_value(
        "cpp_test", {(leg).to_yasf_value()}, std::nullopt);
    } else if constexpr (std::is_same_v<T, GenRule>) {
      return yasf::Value::create_key_value(
        "gen_rule", {(leg).to_yasf_value()}, std::nullopt);
    } else if constexpr (std::is_same_v<T, SystemLib>) {
      return yasf::Value::create_key_value(
        "system_lib", {(leg).to_yasf_value()}, std::nullopt);
    } else if constexpr (std::is_same_v<T, ExternalPackage>) {
      return yasf::Value::create_key_value(
        "external_package", {(leg).to_yasf_value()}, std::nullopt);
    }
  });
}

} // namespace generated_mbuild_parser

// olint-allow: missing-package-namespace
