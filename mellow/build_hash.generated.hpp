#pragma once

#include <set>
#include <string>
#include <variant>
#include <vector>

#include "bee/or_error.hpp"
#include "yasf/serializer.hpp"
#include "yasf/time.hpp"
#include "yasf/to_stringable_mixin.hpp"

namespace mellow {

struct FileHash : public yasf::ToStringableMixin<FileHash> {
  std::string name;
  std::string hash;
  yasf::Time mtime;

  static bee::OrError<FileHash> of_yasf_value(
    const yasf::Value::ptr& config_value);

  yasf::Value::ptr to_yasf_value() const;
};

struct TaskHash : public yasf::ToStringableMixin<TaskHash> {
  std::vector<FileHash> inputs;
  std::vector<FileHash> outputs;
  std::string flags_hash;

  static bee::OrError<TaskHash> of_yasf_value(
    const yasf::Value::ptr& config_value);

  yasf::Value::ptr to_yasf_value() const;
};

} // namespace mellow
