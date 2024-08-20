#pragma once

#include <functional>
#include <memory>
#include <optional>

#include "bee/or_error.hpp"

namespace mellow {

struct ThreadRunner {
 public:
  using ptr = std::shared_ptr<ThreadRunner>;

  virtual ~ThreadRunner();

  static ptr create(const std::optional<int>& workers = std::nullopt);

  virtual void enqueue(
    std::function<bee::OrError<>()>&& f,
    std::function<void(bee::OrError<>&& value)>&& on_done) = 0;

  virtual void close_join() = 0;
};

} // namespace mellow
