#include "generated_build_hash.hpp"

#include <type_traits>

#include "bee/format.hpp"
#include "bee/util.hpp"
#include "yasf/parser_helpers.hpp"
#include "yasf/serializer.hpp"

using PH = yasf::ParserHelper;

namespace generated_build_hash {

////////////////////////////////////////////////////////////////////////////////
// FileHash
//

bee::OrError<FileHash> FileHash::of_yasf_value(const yasf::Value::ptr& value)
{
  if (!value->is_list()) {
    return PH::err("$: Expected list for record", (value));
  }

  std::optional<std::string> output_name;
  std::optional<std::string> output_hash;
  std::optional<bee::Time> output_mtime;

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
    } else if (name == "hash") {
      if (output_hash.has_value()) {
        return PH::err("Field 'hash' is defined more than once", element);
      }
      bail_assign(output_hash, yasf::des<std::string>(kv.value));
    } else if (name == "mtime") {
      if (output_mtime.has_value()) {
        return PH::err("Field 'mtime' is defined more than once", element);
      }
      bail_assign(output_mtime, yasf::des<bee::Time>(kv.value));
    } else {
      return PH::err("No such field in record of type FileHash", element);
    }
  }

  if (!output_name.has_value()) {
    return PH::err("Field 'name' not defined", value);
  }
  if (!output_hash.has_value()) {
    return PH::err("Field 'hash' not defined", value);
  }
  if (!output_mtime.has_value()) {
    return PH::err("Field 'mtime' not defined", value);
  }

  return FileHash{
    .name = std::move(*output_name),
    .hash = std::move(*output_hash),
    .mtime = std::move(*output_mtime),
  };
}

yasf::Value::ptr FileHash::to_yasf_value() const
{
  std::vector<yasf::Value::ptr> fields;
  PH::push_back_field(fields, yasf::ser<std::string>(name), "name");
  PH::push_back_field(fields, yasf::ser<std::string>(hash), "hash");
  PH::push_back_field(fields, yasf::ser<bee::Time>(mtime), "mtime");
  return yasf::Value::create_list(std::move(fields), std::nullopt);
}

////////////////////////////////////////////////////////////////////////////////
// TaskHash
//

bee::OrError<TaskHash> TaskHash::of_yasf_value(const yasf::Value::ptr& value)
{
  if (!value->is_list()) {
    return PH::err("$: Expected list for record", (value));
  }

  std::optional<std::vector<FileHash>> output_inputs;
  std::optional<std::vector<FileHash>> output_outputs;
  std::optional<std::string> output_flags_hash;

  for (const auto& element : value->list()) {
    if (!element->is_key_value()) {
      return PH::err("Expected a key value as a record element", element);
    }

    const auto& kv = element->key_value();
    const std::string& name = kv.key;
    if (name == "inputs") {
      if (output_inputs.has_value()) {
        return PH::err("Field 'inputs' is defined more than once", element);
      }
      bail_assign(output_inputs, yasf::des<std::vector<FileHash>>(kv.value));
    } else if (name == "outputs") {
      if (output_outputs.has_value()) {
        return PH::err("Field 'outputs' is defined more than once", element);
      }
      bail_assign(output_outputs, yasf::des<std::vector<FileHash>>(kv.value));
    } else if (name == "flags_hash") {
      if (output_flags_hash.has_value()) {
        return PH::err("Field 'flags_hash' is defined more than once", element);
      }
      bail_assign(output_flags_hash, yasf::des<std::string>(kv.value));
    } else {
      return PH::err("No such field in record of type TaskHash", element);
    }
  }

  if (!output_inputs.has_value()) {
    return PH::err("Field 'inputs' not defined", value);
  }
  if (!output_outputs.has_value()) {
    return PH::err("Field 'outputs' not defined", value);
  }
  if (!output_flags_hash.has_value()) {
    return PH::err("Field 'flags_hash' not defined", value);
  }

  return TaskHash{
    .inputs = std::move(*output_inputs),
    .outputs = std::move(*output_outputs),
    .flags_hash = std::move(*output_flags_hash),
  };
}

yasf::Value::ptr TaskHash::to_yasf_value() const
{
  std::vector<yasf::Value::ptr> fields;
  PH::push_back_field(
    fields, yasf::ser<std::vector<FileHash>>(inputs), "inputs");
  PH::push_back_field(
    fields, yasf::ser<std::vector<FileHash>>(outputs), "outputs");
  PH::push_back_field(fields, yasf::ser<std::string>(flags_hash), "flags_hash");
  return yasf::Value::create_list(std::move(fields), std::nullopt);
}

} // namespace generated_build_hash

// olint-allow: missing-package-namespace
