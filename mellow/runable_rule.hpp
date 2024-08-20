#pragma once

#include <memory>

#include "bee/or_error.hpp"

namespace mellow {

struct RunableRule {
 public:
  using ptr = std::shared_ptr<RunableRule>;

  RunableRule(const bool is_test);

  virtual ~RunableRule();

  virtual bee::OrError<> run() const = 0;

  bool is_test() const { return _is_test; }

 private:
  bool _is_test;
};

} // namespace mellow
