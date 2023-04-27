// memory budget header

#pragma once

#include <limits>

#include "Except.h"

namespace zezax::red {

/*
  A Budget object limits total allocations during compilation.  The unit
  of measure is the "state", which is easy to account for, but doesn't
  really translate directly to bytes.  Roughly 11-22kB per state on amd64.

  A Budget must live longer than the compilation that uses it.
  Don't share a Budget across threads.  There's no locking here.
*/
class Budget {
public:
  Budget() : avail_(std::numeric_limits<size_t>::max()) {}
  explicit Budget(size_t states) : avail_(states) {}
  ~Budget() = default;
  Budget(const Budget &other) = delete; // copying a budget is double-counting
  Budget(Budget &&other) = default;     // moving is ok
  Budget &operator=(const Budget &rhs) = delete;
  Budget &operator=(Budget &&rhs) = default;

  void init(size_t states) { avail_ = states; }

  void take(size_t states) {
    if (states > avail_)
      throw RedExceptLimit("budget exceeded");
    avail_ -= states;
  }

  void give(size_t states) { avail_ += states; }

private:
  size_t avail_;
};

} // namespace zezax::red
