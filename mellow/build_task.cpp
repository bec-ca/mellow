#include "build_task.hpp"

#include "bee/format.hpp"
#include "hash_checker.hpp"
#include "thread_runner.hpp"

#include <functional>
#include <memory>
#include <set>
#include <string>

using bee::FilePath;
using std::function;
using std::set;
using std::shared_ptr;
using std::string;

namespace mellow {

////////////////////////////////////////////////////////////////////////////////
// BuildTask
//

BuildTask::BuildTask(
  const PackagePath& key,
  FilePath root_build_dir,
  function<bee::OrError<bee::Unit>()> run,
  set<FilePath> inputs,
  set<FilePath> outputs,
  const string& non_file_inputs_key,
  const ProgressUI::ptr& progress_ui)
    : _key(key),
      _root_build_dir(root_build_dir),
      _run(std::move(run)),
      _inputs(std::move(inputs)),
      _outputs(std::move(outputs)),
      _non_file_inputs_key(non_file_inputs_key),
      _progress_ui(progress_ui),
      _task_progress(progress_ui->add_task(key))
{}

BuildTask::ptr BuildTask::create(
  const BuildTask::Args& args, const ProgressUI::ptr& progress_ui)
{
  return make_shared<BuildTask>(
    args.key,
    args.root_build_dir,
    args.run,
    args.inputs,
    args.outputs,
    args.non_file_inputs_key,
    progress_ui);
}

void BuildTask::mark_done(
  const shared_ptr<ThreadRunner>& runner, bool force_build)
{
  assert(!_done);
  _done = true;
  auto ptr = shared_from_this();
  for (const auto& t : (_depended_on)) {
    t->dependency_done(ptr, runner, force_build);
  }
}

const bee::OrError<bee::Unit>& BuildTask::error() const { return _error; }

bool BuildTask::is_done() const { return _done; }

const PackagePath& BuildTask::key() const { return _key; }

void BuildTask::depends_on(const ptr& dependent, const ptr& depender)
{
  depender->add_depended_on(dependent);
  dependent->add_depends_on(depender);
}

void BuildTask::mark_error(bee::Error error)
{
  assert(!_done);
  _done = true;
  _error = std::move(error);
}

bool BuildTask::is_runnable() const { return _depends_on.empty(); }

bee::OrError<bee::Unit> BuildTask::_do_run(bool force_build)
{
  _progress_ui->task_started(_task_progress);
  bool cached = true;
  auto result = [&]() -> bee::OrError<bee::Unit> {
    if (_run != nullptr) {
      auto hash_cache_filename =
        _key.append_no_sep(".hash").to_filesystem(_root_build_dir);
      auto hash_checker = HashChecker::create(
        hash_cache_filename, _inputs, _outputs, _non_file_inputs_key);
      if (force_build || !hash_checker.is_up_to_date()) {
        bail_unit(_run());
        cached = false;
      }
      hash_checker.write_updated_hashes();
    } else {
      if (force_build) { cached = false; }
    }
    return bee::ok();
  }();
  _progress_ui->task_done(_task_progress, cached);

  return result;
}

void BuildTask::enqueue_if_runnable(
  const ThreadRunner::ptr& runner, bool force_build)
{
  if (!is_runnable()) { return; }

  auto task = shared_from_this();
  runner->enqueue<bee::OrError<bee::Unit>>(
    [=]() { return task->_do_run(force_build); },
    [=](bee::OrError<bee::Unit> result) {
      if (result.is_error()) {
        task->mark_error(bee::Error::format(
          "$ failed: $", task->_key, std::move(result.error())));
      } else {
        task->mark_done(runner, force_build);
      }
    });
}

void BuildTask::dependency_done(
  const ptr& dep, const shared_ptr<ThreadRunner>& runner, bool force_build)
{
  _depends_on.erase(dep);
  enqueue_if_runnable(runner, force_build);
}

void BuildTask::add_depends_on(ptr task)
{
  _depends_on.insert(std::move(task));
}

void BuildTask::add_depended_on(ptr task)
{
  _depended_on.insert(std::move(task));
}

const set<FilePath> BuildTask::outputs() const { return _outputs; }
const set<FilePath> BuildTask::inputs() const { return _inputs; }

} // namespace mellow
