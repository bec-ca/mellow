#include "task_manager.hpp"

#include <map>

#include "package_path.hpp"

#include "bee/print.hpp"

namespace mellow {
namespace {

constexpr auto visual_double_sep =
  "========================================================================"
  "========";

constexpr auto visual_single_sep = "-------------------------------------------"
                                   "-------------------------------------";

struct Artifact {
 public:
  using ptr = std::shared_ptr<Artifact>;

  BuildTask::ptr producer;
  std::set<BuildTask::ptr> consumers;
};

struct Summary {
  size_t num_tasks = 0;
  size_t ran_tasks = 0;
  size_t cached_tasks = 0;
  std::set<PackagePath> didnt_run_tasks{};
  std::map<PackagePath, bee::Error> failed_tasks{};

  void show()
  {
    for (auto& p : failed_tasks) {
      P(visual_double_sep);
      P(p.second);
    }
    P(visual_double_sep);
    P("Build summary:");
    P("Num tasks: $", num_tasks);
    if (ran_tasks > 0) { P("Tasks ran: $", ran_tasks); }
    if (cached_tasks > 0) { P("Cached tasks: $", cached_tasks); }
    if (!failed_tasks.empty()) { P("Failed tasks: $", failed_tasks.size()); }
    if (!didnt_run_tasks.empty()) {
      P("Didn't run: $", didnt_run_tasks.size());
    }
    if (!failed_tasks.empty()) {
      P(visual_single_sep);
      for (auto& p : failed_tasks) { P("$ FAILED", p.first); }
    } else if (!didnt_run_tasks.empty()) {
      P(visual_single_sep);
      for (auto& name : failed_tasks) { P("$ DIDN'T RUN", name); }
    }
    P(visual_double_sep);
  }

  bee::OrError<> result()
  {
    if (!failed_tasks.empty()) { return bee::Error("Some tasks failed"); }
    if (!didnt_run_tasks.empty()) {
      return bee::Error("Some tasks didn't run");
    }
    return bee::ok();
  }
};

struct TaskManagerImpl : public TaskManager {
  TaskManagerImpl(const Args& args)
      : _args(args), _progress_ui(std::make_shared<ProgressUI>())
  {}

  virtual ~TaskManagerImpl()
  {
    for (const auto& task : _tasks) { task->clear(); }
  }

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
      if (artifact->producer != nullptr) {
        raise_error(
          "Multiple rules producing the same output file. Rules:$,$ Output:$",
          task->key(),
          artifact->producer->key(),
          output);
      }
      assert(artifact->producer == nullptr);
      artifact->producer = task;
    }
  }

  Summary create_summary() const
  {
    Summary s{.num_tasks = _tasks.size()};
    for (const auto& task : _tasks) {
      const auto& status = task->status();
      const auto& key = task->key();
      if (status.error.is_error()) {
        s.failed_tasks.emplace(key, status.error.error());
      };
      if (!status.done) { s.didnt_run_tasks.insert(key); }
      if (status.done && !status.cached) { s.ran_tasks++; }
      if (status.cached) { s.cached_tasks++; }
    }
    return s;
  }

  virtual bee::OrError<> run() override
  {
    for (auto& [_, artifact] : _artifacts) {
      if (artifact->producer == nullptr) { continue; }
      for (auto& consumer : artifact->consumers) {
        BuildTask::add_dependency(consumer, artifact->producer);
      }
    }

    auto runner = ThreadRunner::create();

    for (auto& task : _tasks) {
      task->enqueue_if_runnable(runner, _args.force_build, _args.force_test);
    }

    runner->close_join();

    auto summary = create_summary();
    summary.show();
    return summary.result();
  }

  Artifact::ptr get_artifact(const bee::FilePath& name)
  {
    auto it = _artifacts.find(name);
    if (it == _artifacts.end()) {
      it = _artifacts.emplace(name, std::make_shared<Artifact>()).first;
    }
    return it->second;
  }

 private:
  const Args _args;
  ProgressUI::ptr _progress_ui;

  std::vector<BuildTask::ptr> _tasks;

  std::map<bee::FilePath, Artifact::ptr> _artifacts;
};

} // namespace

////////////////////////////////////////////////////////////////////////////////
// TaskManager
//

TaskManager::~TaskManager() {}

TaskManager::ptr TaskManager::create(const Args& args)
{
  return std::make_shared<TaskManagerImpl>(args);
}

} // namespace mellow
