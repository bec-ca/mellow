#include <atomic>
#include <thread>

#include "thread_runner.hpp"

#include "bee/testing.hpp"

using std::vector;

namespace mellow {
namespace {

TEST(basic)
{
  auto runner = ThreadRunner::create(4);
  auto main_id = std::this_thread::get_id();
  auto same_as_main = [main_id]() {
    return main_id == std::this_thread::get_id() ? "running on main thread"
                                                 : "running on another thread";
  };
  P("running test: $", same_as_main());
  runner->enqueue(
    [=] -> bee::OrError<> {
      P("running task: $", same_as_main());
      return bee::ok();
    },
    [=](const auto& res) { P("done running: $ res:$", same_as_main(), res); });
  runner->close_join();
}

TEST(many)
{
  auto runner = ThreadRunner::create(4);
  int counter_done = 0;
  std::atomic<int> counter_ran{0};
  int num_jobs = 1000;
  for (int i = 0; i < num_jobs; i++) {
    runner->enqueue(
      [&] mutable -> bee::OrError<> {
        counter_ran++;
        return bee::ok();
      },
      [&](const auto&) mutable { counter_done++; });
  }
  runner->close_join();

  P("num_jobs: $", num_jobs);
  P("counter_ran: $", counter_ran.load());
  P("counter_done: $", counter_done);
}

TEST(failure)
{
  auto runner = ThreadRunner::create(4);
  const int num_jobs = 10;
  std::vector<std::string> output(num_jobs);
  for (int i = 0; i < num_jobs; i++) {
    runner->enqueue(
      [=]() -> bee::OrError<> {
        if (i % 2 == 0)
          return EF("failed");
        else
          return bee::ok();
      },
      [i, &output](const auto& res) mutable { output[i] = F("$ $", i, res); });
  }
  runner->close_join();
  for (auto v : output) { P(v); }
}

} // namespace
} // namespace mellow
