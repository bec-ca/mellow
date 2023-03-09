#pragma once

#include "yasf/value.hpp"

#include "generated_build_config.hpp"

#include <filesystem>

namespace mellow {

namespace bc = generated_build_config;

// BuildConfig is a temporary config, system specific required for a build to
// work. Eg, path to compiler, additional flags for unusual header locations,
// etc.

struct BuildConfig {
 public:
  BuildConfig();

  ~BuildConfig();

  yasf::Value::ptr to_yasf_value() const;

  static bee::OrError<BuildConfig> of_yasf_value(const yasf::Value::ptr& value);

  static bee::OrError<BuildConfig> load_from_file(
    const std::filesystem::path& filename);

  bee::OrError<bee::Unit> write_to_file(
    const std::filesystem::path& filename) const;

  bc::Cpp cpp_config() const;

  std::vector<bc::Rule> rules;
};

} // namespace mellow
