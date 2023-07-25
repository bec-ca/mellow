#pragma once

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>

#include "package_path.hpp"
#include "progress_ui.hpp"
#include "thread_runner.hpp"

#include "bee/file_path.hpp"

namespace mellow {

struct BuildTask : public std::enable_shared_from_this<BuildTask> {
 public:
  using ptr = std::shared_ptr<BuildTask>;

  struct Args {
    PackagePath key;
    bee::FilePath root_build_dir;
    std::function<bee::OrError<>()> run = nullptr;
    std::set<bee::FilePath> inputs = {};
    std::set<bee::FilePath> outputs = {};
    std::string non_file_inputs_key = "";
  };

  BuildTask(
    const PackagePath& key,
    bee::FilePath root_build_dir,
    std::function<bee::OrError<>()> run,
    std::set<bee::FilePath> inputs,
    std::set<bee::FilePath> outputs,
    const std::string& non_file_inputs_key,
    const ProgressUI::ptr& progress_ui);

  static ptr create(const Args& args, const ProgressUI::ptr& progress_ui);

  void mark_done(const ThreadRunner::ptr& runner, bool force_build);

  void enqueue_if_runnable(const ThreadRunner::ptr& runner, bool force_build);

  const bee::OrError<>& error() const;

  bool is_done() const;

  const PackagePath& key() const;

  const std::set<bee::FilePath> outputs() const;

  const std::set<bee::FilePath> inputs() const;

  static void depends_on(const ptr& dependent, const ptr& depender);

  bool is_runnable() const;

  void clear();

  bool did_start() const { return _did_start; }

  const std::set<ptr>& depends_on() const { return _depends_on; }

 private:
  void mark_error(bee::Error&& error);

  void add_depends_on(ptr task);

  void add_depended_on(ptr task);

  bee::OrError<> _do_run(bool force_build);

  PackagePath _key;

  bee::FilePath _root_build_dir;

  std::function<bee::OrError<>()> _run;

  std::set<bee::FilePath> _inputs;
  std::set<bee::FilePath> _outputs;
  std::string _non_file_inputs_key;

  std::set<ptr> _depends_on;
  std::set<ptr> _depended_on;
  bool _done = false;

  bool _did_start = false;

  bee::OrError<> _error = bee::ok();
  ProgressUI::ptr _progress_ui;
  TaskProgress::ptr _task_progress;
};

} // namespace mellow
