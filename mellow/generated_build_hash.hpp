#pragma once

#include <set>
#include <string>
#include <variant>
#include <vector>

#include "bee/error.hpp"
#include "bee/time.hpp"
#include "yasf/serializer.hpp"
#include "yasf/to_stringable_mixin.hpp"

namespace generated_build_hash {

struct FileHash : public yasf::ToStringableMixin<FileHash> {
  std::string name;
  std::string hash;
  bee::Time mtime;

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

} // namespace generated_build_hash

// olint-allow: missing-package-namespace
