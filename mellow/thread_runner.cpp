#include "thread_runner.hpp"

#include <thread>

#include "bee/print.hpp"
#include "bee/queue.hpp"

using std::function;
using std::thread;

namespace mellow {
namespace {

struct ThreadRunnerImpl final : public ThreadRunner {
 public:
  ThreadRunnerImpl(const int workers)
  {
    P("Using $ workers", workers);
    for (int i = 0; i < workers; i++) {
      _workers.emplace_back([job_queue = _job_queue] {
        while (true) {
          auto f = job_queue->pop();
          if (!f.has_value()) { return; }
          (*f)();
        }
      });
    }
  }

  virtual ~ThreadRunnerImpl() { close(); }

  void enqueue(
    std::function<bee::OrError<>()>&& f,
    std::function<void(bee::OrError<>&& value)>&& on_done) override
  {
    _job_queue->push([f = std::move(f),
                      on_done = std::move(on_done),
                      on_done_queue = this->_on_done_queue]() mutable {
      auto result = bee::try_with(std::move(f));
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
    _pending++;
  }

  void close_join() override
  {
    wait_all_done();
    close();
  }

 private:
  void wait_all_done()
  {
    while (_pending > 0) {
      auto f = _on_done_queue->pop();
      if (!f.has_value()) { break; }
      std::move (*f)();
      _pending--;
    }
  }

  void close()
  {
    _job_queue->close();
    for (auto& worker : _workers) {
      if (worker.joinable()) { worker.join(); }
    }
  }

  using queue_type = bee::Queue<std::function<void()>>;

  std::shared_ptr<queue_type> _job_queue = std::make_shared<queue_type>();
  std::shared_ptr<queue_type> _on_done_queue = std::make_shared<queue_type>();
  std::vector<std::thread> _workers;

  int _pending = 0;
};

} // namespace

ThreadRunner::~ThreadRunner() {}

ThreadRunner::ptr ThreadRunner::create(const std::optional<int>& workers)
{
  return std::make_shared<ThreadRunnerImpl>(
    workers.value_or(thread::hardware_concurrency()));
}

} // namespace mellow
