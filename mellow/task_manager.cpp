#include "task_manager.hpp"

#include "package_path.hpp"

#include "bee/string_util.hpp"

using std::make_shared;
using std::shared_ptr;

namespace mellow {
namespace {

struct Artifact {
 public:
  using ptr = shared_ptr<Artifact>;

  BuildTask::ptr producer;
  std::set<BuildTask::ptr> consumers;
};

struct TaskManagerImpl : public TaskManager {
  TaskManagerImpl() : _progress_ui(make_shared<ProgressUI>()) {}

  virtual ~TaskManagerImpl() {}

  virtual void create_task(const BuildTask::Args& args) override
  {
    auto task = BuildTask::create(args, _progress_ui);
    _tasks.push_back(task);

    for (const auto& input : task->inputs()) {
      auto artifact = get_artifact(input);
      artifact->consumers.insert(task);
    }

    for (const auto& output : task->outputs()) {
      auto artifact = get_artifact(output);
      assert(artifact->producer == nullptr);
      artifact->producer = task;
    }
  }

  virtual bee::OrError<> run(bool force_build) override
  {
    for (auto& [_, artifact] : _artifacts) {
      if (artifact->producer == nullptr) { continue; }
      for (auto& consumer : artifact->consumers) {
        BuildTask::depends_on(consumer, artifact->producer);
      }
    }

    auto runner = make_shared<ThreadRunner>(ThreadRunner::num_cpus());

    for (auto& task : _tasks) {
      task->enqueue_if_runnable(runner, force_build);
    }

    runner->close_join();

    std::vector<bee::Error> errors;
    for (const auto& task : _tasks) {
      if (task->error().is_error()) {
        errors.push_back(task->error().error());
      };
    }

    if (!errors.empty()) {
      for (const auto& err : errors) { P(err.msg()); }
      return bee::Error("Build failed");
    }

    std::set<std::string> tasks_that_didnt_run;
    for (const auto& task : _tasks) {
      if (!task->is_done()) {
        tasks_that_didnt_run.insert(task->key().to_string());
      }
    }

    for (const auto& task : _tasks) { task->clear(); }

    if (!tasks_that_didnt_run.empty()) {
      return bee::Error::fmt(
        "Build tasks did not run:\n$", bee::join(tasks_that_didnt_run, "\n"));
    }

    return bee::ok();
  }

  Artifact::ptr get_artifact(const bee::FilePath& name)
  {
    auto it = _artifacts.find(name);
    if (it == _artifacts.end()) {
      it = _artifacts.emplace(name, make_shared<Artifact>()).first;
    }
    return it->second;
  }

 private:
  ProgressUI::ptr _progress_ui;

  std::vector<BuildTask::ptr> _tasks;

  std::map<bee::FilePath, Artifact::ptr> _artifacts;
};

} // namespace

////////////////////////////////////////////////////////////////////////////////
// TaskManager
//

TaskManager::~TaskManager() {}

TaskManager::ptr TaskManager::create()
{
  return make_shared<TaskManagerImpl>();
}

} // namespace mellow
