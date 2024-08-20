#pragma once

#include <memory>
#include <set>
#include <string>

#include "package_path.hpp"
#include "progress_ui.hpp"
#include "runable_rule.hpp"
#include "thread_runner.hpp"

#include "bee/file_path.hpp"

namespace mellow {

struct BuildTask {
 public:
  using ptr = std::shared_ptr<BuildTask>;

  struct Status {
    bool started{false};
    bool cached{false};
    bool done{false};
    bee::OrError<> error{};
  };

  struct Args {
    PackagePath key;
    bee::FilePath root_build_dir;
    RunableRule::ptr run = nullptr;
    std::set<bee::FilePath> inputs{};
    std::set<bee::FilePath> outputs{};
    std::string non_file_inputs_key{};
  };

  virtual ~BuildTask();

  // Create
  static ptr create(const Args& args, const ProgressUI::ptr& progress_ui);

  // Getters
  virtual const Status& status() const = 0;
  virtual const PackagePath& key() const = 0;
  virtual const std::set<bee::FilePath>& outputs() const = 0;
  virtual const std::set<bee::FilePath>& inputs() const = 0;

  // Core methods
  virtual void enqueue_if_runnable(
    const ThreadRunner::ptr& runner, bool force_build, bool force_test) = 0;

  // Used to remove shared ptr cycles
  virtual void clear() = 0;

  static void add_dependency(const ptr& dependent, const ptr& dependency);

 protected:
  virtual void add_dependent(const ptr& dep) = 0;
  virtual void add_dependency(const ptr& dep) = 0;
};

} // namespace mellow
