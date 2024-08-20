#pragma once

#include "build_task.hpp"

namespace mellow {

struct TaskManager {
  using ptr = std::shared_ptr<TaskManager>;

  struct Args {
    bool force_build;
    bool force_test;
  };

  virtual ~TaskManager();

  virtual void create_task(const BuildTask::Args& args) = 0;

  virtual bee::OrError<> run() = 0;

  static ptr create(const Args& args);
};

} // namespace mellow
