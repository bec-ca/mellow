#include "thread_runner.hpp"

#include "bee/format.hpp"

using std::function;
using std::thread;

namespace mellow {

ThreadRunner::ThreadRunner(int workers)
{
  bee::print_line("Using $ workers", workers);
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

ThreadRunner::~ThreadRunner() { _job_queue->close(); }

void ThreadRunner::wait_all_done()
{
  while (_pending > 0) {
    auto f = _on_done_queue->pop();
    if (!f.has_value()) { break; }
    (*f)();
    _pending--;
  }
}

void ThreadRunner::_enqueue(function<void()> f)
{
  _job_queue->push(std::move(f));
  _pending++;
}

void ThreadRunner::close_join()
{
  wait_all_done();
  _job_queue->close();
  for (auto& worker : _workers) { worker.join(); }
}

int ThreadRunner::num_cpus() { return thread::hardware_concurrency(); }

} // namespace mellow
