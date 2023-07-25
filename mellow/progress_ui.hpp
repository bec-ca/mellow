#pragma once

#include <memory>
#include <mutex>
#include <optional>
#include <set>

#include "package_path.hpp"

#include "bee/time.hpp"

namespace mellow {

////////////////////////////////////////////////////////////////////////////////
// TaskProgress
//

struct TaskProgress {
 public:
  using ptr = std::shared_ptr<TaskProgress>;

  TaskProgress(const PackagePath& name);
  ~TaskProgress();

  void set_start_time(const bee::Time& time);

  const PackagePath& name() const;

  std::optional<bee::Time> start_time() const;

  void set_done();

  bool done() const;

 private:
  PackagePath _name;
  std::optional<bee::Time> _start_time;

  bool _done = false;
};

////////////////////////////////////////////////////////////////////////////////
// ProgressUI
//

struct ProgressUI {
 public:
  using ptr = std::shared_ptr<ProgressUI>;

  ProgressUI();
  ~ProgressUI();

  TaskProgress::ptr add_task(const PackagePath& name);

  void task_started(const TaskProgress::ptr& task);
  void task_done(const TaskProgress::ptr& task, bool cached);

 private:
  std::set<TaskProgress::ptr> _all_tasks;

  void _show_running_tasks();

  void _add_running_task(const TaskProgress::ptr& task);
  void _remove_running_task(const TaskProgress::ptr& task);

  int _finished_tasks = 0;
  int _running_tasks = 0;
  int _cached_tasks = 0;

  std::vector<std::string> _shown_lines;
  std::vector<TaskProgress::ptr> _running_task_set;

  std::mutex _lock;

  bool _is_tty;
};

} // namespace mellow
