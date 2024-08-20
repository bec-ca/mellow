#include "build_task.hpp"

#include <memory>
#include <set>
#include <string>

#include "hash_checker.hpp"
#include "package_path.hpp"
#include "runable_rule.hpp"
#include "thread_runner.hpp"

namespace mellow {
namespace {

////////////////////////////////////////////////////////////////////////////////
// BuildTaskImpl
//

HashChecker create_hash_checker(const BuildTask::Args& args)
{
  const auto hash_cache_filename =
    args.key.append_no_sep(".hash").to_filesystem(args.root_build_dir);
  return HashChecker::create(
    hash_cache_filename, args.inputs, args.outputs, args.non_file_inputs_key);
}

struct BuildTaskImpl final : BuildTask,
                             std::enable_shared_from_this<BuildTaskImpl> {
 public:
  BuildTaskImpl(const Args& args, const ProgressUI::ptr& progress_ui)
      : _key(args.key),
        _root_build_dir(args.root_build_dir),
        _run(args.run),
        _inputs(args.inputs),
        _outputs(args.outputs),
        _non_file_inputs_key(args.non_file_inputs_key),
        _progress_ui(progress_ui),
        _task_progress(progress_ui->add_task(args.key)),
        _hash_checker(create_hash_checker(args))
  {}

  // Getters

  virtual const Status& status() const override { return _status; }
  const PackagePath& key() const override { return _key; }
  const std::set<bee::FilePath>& outputs() const override { return _outputs; }
  const std::set<bee::FilePath>& inputs() const override { return _inputs; }

  // Core methods

  void enqueue_if_runnable(
    const ThreadRunner::ptr& runner,
    const bool force_build,
    const bool force_test) override
  {
    if (!is_runnable()) { return; }

    const auto task = shared_from_this();
    runner->enqueue(
      [=]() { return task->do_run(force_build, force_test); },
      [=](bee::OrError<>&& result) {
        if (result.is_error()) {
          task->mark_error(
            bee::Error::fmt("$ failed: $", task->_key, result.error()));
        } else {
          task->mark_done(runner, force_build, force_test);
        }
      });
  }

  void clear() override
  {
    _dependencies.clear();
    _dependents.clear();
  }

 protected:
  void add_dependent(const ptr& t) override { _dependents.insert(t); }
  void add_dependency(const ptr& t) override { _dependencies.insert(t); }

 private:
  void mark_done(
    const ThreadRunner::ptr& runner,
    const bool force_build,
    const bool force_test)
  {
    assert(!_status.done);
    _status.done = true;
    for (const auto& t : _dependents) {
      t->enqueue_if_runnable(runner, force_build, force_test);
    }
  }

  bool is_runnable() const
  {
    for (const auto& dep : _dependencies) {
      const auto& s = dep->status();
      if (!s.done || s.error.is_error()) { return false; }
    }
    return true;
  }

  void mark_error(bee::Error&& error)
  {
    assert(!_status.done);
    _status.done = true;
    _status.error = std::move(error);
  }

  bool needs_to_run(const bool force_build, const bool force_test)
  {
    if (force_build) return true;
    if (force_test && _run->is_test()) return true;
    return !_hash_checker.is_up_to_date();
  }

  bee::OrError<> do_run(const bool force_build, const bool force_test)
  {
    _status.started = true;
    _progress_ui->task_started(_task_progress);
    auto result = [&]() -> bee::OrError<> {
      if (needs_to_run(force_build, force_test)) {
        bail_unit(_run->run());
      } else {
        _status.cached = true;
      }
      _hash_checker.write_updated_hashes();
      return bee::ok();
    }();
    _progress_ui->task_done(_task_progress, _status.cached);

    return result;
  }

  const PackagePath _key;
  const bee::FilePath _root_build_dir;
  const RunableRule::ptr _run;

  const std::set<bee::FilePath> _inputs;
  const std::set<bee::FilePath> _outputs;
  const std::string _non_file_inputs_key;

  const ProgressUI::ptr _progress_ui;
  const TaskProgress::ptr _task_progress;

  std::set<ptr> _dependents;
  std::set<ptr> _dependencies;

  HashChecker _hash_checker;

  Status _status;
};

} // namespace

BuildTask::~BuildTask() {}

BuildTask::ptr BuildTask::create(
  const Args& args, const ProgressUI::ptr& progress_ui)
{
  return make_shared<BuildTaskImpl>(args, progress_ui);
}

void BuildTask::add_dependency(const ptr& dependent, const ptr& dependency)
{
  dependency->add_dependent(dependent);
  dependent->add_dependency(dependency);
}

} // namespace mellow
