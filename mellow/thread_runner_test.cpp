#include <atomic>
#include <cmath>
#include <exception>
#include <thread>

#include "thread_runner.hpp"

#include "bee/format.hpp"
#include "bee/testing.hpp"

using std::pair;
using std::vector;

namespace mellow {
namespace {

TEST(basic)
{
  ThreadRunner runner(4);
  auto main_id = std::this_thread::get_id();
  auto same_as_main = [main_id]() {
    return main_id == std::this_thread::get_id() ? "running on main thread"
                                                 : "running on another thread";
  };
  P("running test: $", same_as_main());
  runner.enqueue(
    [=] { P("running task: $", same_as_main()); },
    [=] { P("done running: $", same_as_main()); });
  runner.close_join();
}

TEST(many)
{
  ThreadRunner runner(4);
  int counter_done = 0;
  std::atomic<int> counter_ran{0};
  int num_jobs = 1000;
  for (int i = 0; i < num_jobs; i++) {
    runner.enqueue(
      [&]() mutable { counter_ran++; }, [&]() mutable { counter_done++; });
  }
  runner.close_join();

  P("num_jobs: $", num_jobs);
  P("counter_ran: $", counter_ran.load());
  P("counter_done: $", counter_done);
}

TEST(return_value)
{
  ThreadRunner runner(4);
  int num_jobs = 100;
  vector<pair<int, double>> output;
  for (int i = 0; i < num_jobs; i++) {
    runner.enqueue<pair<int, double>>(
      [=]() mutable { return pair<int, double>(i, sqrt(i)); },
      [&](pair<int, double> value) mutable { output.push_back(value); });
  }
  runner.close_join();
  sort(output.begin(), output.end());
  for (auto v : output) { P("$ $", v.first, v.second); }
}

} // namespace
} // namespace mellow
