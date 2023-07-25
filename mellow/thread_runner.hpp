#pragma once

#include <functional>
#include <stdexcept>
#include <thread>

#include "bee/queue.hpp"

namespace mellow {

struct ThreadRunner {
  using ptr = std::shared_ptr<ThreadRunner>;
  ThreadRunner(int workers);

  ~ThreadRunner();

  void enqueue(std::function<void()> f, std::function<void()> on_done)
  {
    _enqueue([f = std::move(f),
              on_done = std::move(on_done),
              on_done_queue = this->_on_done_queue]() mutable {
      f();
      on_done_queue->push(std::move(on_done));
    });
  }

  template <class R>
  void enqueue(std::function<R()> f, std::function<void(R value)> on_done)
  {
    _enqueue([f = std::move(f),
              on_done = std::move(on_done),
              on_done_queue = this->_on_done_queue]() mutable {
      bee::OrError<R> result = bee::try_with(std::move(f));
      on_done_queue->push(
        [on_done = std::move(on_done), result = std::move(result)]() mutable {
          if (result.is_error()) {
            raise_error(
              "Unexpected exception thrown from runner: $",
              result.error().full_msg());
          } else {
            on_done(std::move(result.value()));
          }
        });
    });
  }

  void wait_all_done();

  void close_join();

  static int num_cpus();

 private:
  void _enqueue(std::function<void()>&& f);

  void _close();

  using queue_type = bee::Queue<std::function<void()>>;

  std::shared_ptr<queue_type> _job_queue = std::make_shared<queue_type>();

  std::shared_ptr<queue_type> _on_done_queue = std::make_shared<queue_type>();

  std::vector<std::thread> _workers;

  int _pending = 0;
};

} // namespace mellow
