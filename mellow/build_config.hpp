#pragma once

#include "build_config.generated.hpp"

#include "bee/file_path.hpp"
#include "yasf/value.hpp"

namespace mellow {

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
    const bee::FilePath& filename);

  bee::OrError<> write_to_file(const bee::FilePath& filename) const;

  generated::Cpp cpp_config() const;

  std::vector<generated::Rule> rules;
};

} // namespace mellow
