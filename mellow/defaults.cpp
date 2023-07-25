#include "defaults.hpp"

#include "bee/file_path.hpp"

using bee::FilePath;

namespace mellow {

FilePath Defaults::external_packages_dir(const FilePath& output_dir)
{
  return output_dir / "external-packages";
}

FilePath Defaults::output_dir() { return FilePath::of_string("build"); }

} // namespace mellow
