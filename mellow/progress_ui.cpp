#include "progress_ui.hpp"

#include <iostream>
#include <mutex>

#include "bee/fd.hpp"

using bee::Time;
using std::lock_guard;
using std::make_shared;
using std::optional;
using std::string;
using std::vector;

namespace mellow {

////////////////////////////////////////////////////////////////////////////////
// TaskProgress
//

TaskProgress::TaskProgress(const PackagePath& name) : _name(name) {}

TaskProgress::~TaskProgress() {}

const PackagePath& TaskProgress::name() const { return _name; }

void TaskProgress::set_start_time(const Time& time) { _start_time = time; }

optional<Time> TaskProgress::start_time() const { return _start_time; }

void TaskProgress::set_done() { _done = true; }

bool TaskProgress::done() const { return _done; }

////////////////////////////////////////////////////////////////////////////////
// ProgressUI
//

ProgressUI::ProgressUI() : _is_tty(bee::FD::stdout_filedesc()->is_tty()) {}
ProgressUI::~ProgressUI() {}

TaskProgress::ptr ProgressUI::add_task(const PackagePath& name)
{
  lock_guard guard(_lock);
  auto task = make_shared<TaskProgress>(name);
  _all_tasks.insert(task);
  return task;
}

void ProgressUI::task_started(const TaskProgress::ptr& task)
{
  lock_guard guard(_lock);
  if (_all_tasks.count(task) == 0) { assert(false && "Unknown task"); }
  if (task->start_time().has_value()) {
    assert(false && "Task already started");
  }
  if (task->done()) { assert(false && "Task already done"); }
  auto now = Time::monotonic();
  task->set_start_time(now);
  _running_tasks++;
  _add_running_task(task);
  _show_running_tasks();
}

void ProgressUI::task_done(const TaskProgress::ptr& task, bool cached)
{
  lock_guard guard(_lock);
  if (_all_tasks.count(task) == 0) { assert(false && "Unknown task"); }
  if (task->done()) { assert(false && "Task already done"); }
  auto start = task->start_time();
  if (!start.has_value()) { assert(false && "Task didn't start"); }
  auto took = Time::monotonic() - *start;
  if (cached) { _cached_tasks++; }
  _finished_tasks++;
  _running_tasks--;
  if (!_is_tty && !cached) {
    P("$ {.2} ($/$)", task->name(), took, _finished_tasks, _all_tasks.size());
  }
  task->set_done();
  _remove_running_task(task);
  _show_running_tasks();
}

void ProgressUI::_add_running_task(const TaskProgress::ptr& task)
{
  for (auto& spot : _running_task_set) {
    if (spot == nullptr) {
      spot = task;
      return;
    }
  }
  _running_task_set.push_back(task);
}

void ProgressUI::_remove_running_task(const TaskProgress::ptr& task)
{
  for (auto& spot : _running_task_set) {
    if (spot == task) {
      spot = nullptr;
      return;
    }
  }
  assert(false && "Tried to remove a task that wasn't there");
}

void ProgressUI::_show_running_tasks()
{
  if (!_is_tty) return;
  vector<string> will_show;

  for (const auto& task : _running_task_set) {
    string line = "*";
    if (task != nullptr) {
      line += ' ';
      line += task->name().to_string();
    }
    will_show.push_back(line);
  }
  will_show.push_back(
    F("Todo:$/$ Ran:$ Cached:$",
      _all_tasks.size() - _finished_tasks,
      _all_tasks.size(),
      _finished_tasks - _cached_tasks,
      _cached_tasks));

  string buffer;
  // move cursor back up and hide it
  buffer += F("\x1b[$A\x1b[?25l", _shown_lines.size());
  for (int i = 0; i < int(will_show.size()) || i < int(_shown_lines.size());
       i++) {
    string line;
    if (i < int(will_show.size())) {
      line = will_show[i];
    } else {
      will_show.push_back("");
    }
    if (i < int(_shown_lines.size())) {
      size_t size = _shown_lines[i].size();
      if (size > line.size()) { line.resize(size, ' '); }
    }
    buffer += line;
    buffer += '\n';
  }
  // leave an empty line at the beginning
  buffer += "\x1b[?25h"; // show cursor again

  bee::FD::stdout_filedesc()->write(buffer);
  _shown_lines = will_show;
}

} // namespace mellow
