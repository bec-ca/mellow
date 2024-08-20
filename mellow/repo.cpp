#include "repo.hpp"

#include "bee/filesystem.hpp"

namespace mellow {

bee::OrError<bee::FilePath> Repo::root_dir(const bee::FilePath& starting_dir)
{
  bee::FilePath path = starting_dir;
  while (true) {
    if (bee::FileSystem::exists(path / "mellowrc")) { return path; }
    auto parent = path.parent();
    if (parent == path) { return EF("mellowrc not found"); }
    path = std::move(parent);
  }
}

} // namespace mellow
