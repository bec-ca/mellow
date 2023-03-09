#pragma once

#include "build_task.hpp"

namespace mellow {

struct TaskManager {
  using ptr = std::shared_ptr<TaskManager>;

  virtual ~TaskManager();

  virtual void create_task(const BuildTask::Args& args) = 0;

  virtual bee::OrError<bee::Unit> run(bool force_build) = 0;

  static ptr create();
};

} // namespace mellow
