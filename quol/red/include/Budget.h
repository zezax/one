/* Budget.h - resource budget header

   A Budget object limits consumption of certain resources during
   compilation, generally memory.  The available settings are:

   States - This unit of measure encompasses NFA states and DFA states.
            It's easy to account for, but doesn't really translate
            directly to bytes.  Roughly 11-22kB per state on amd64.

   Parens - Limits nesting of the recursive descent parser.  Basically,
            this counts levels of parentheses in order to prevent
            stack overflow.

   A Budget must live longer than the compilation that uses it.
   Don't share a Budget across threads.  There's no locking here.

   In general, RedExceptLimit is thrown when the budget is exceeded.
 */

#pragma once

#include <limits>

#include "Except.h"

namespace zezax::red {

class Budget {
public:
  Budget()
    : statesAvail_(std::numeric_limits<size_t>::max()),
      parensAvail_(std::numeric_limits<size_t>::max()) {}
  Budget(size_t states, size_t parens)
    : statesAvail_(states), parensAvail_(parens) {}
  ~Budget() = default;
  Budget(const Budget &other) = delete; // copying a budget is double-counting
  Budget(Budget &&other) = default;     // moving is ok
  Budget &operator=(const Budget &rhs) = delete;
  Budget &operator=(Budget &&rhs) = default;

  void initStates(size_t states) { statesAvail_ = states; }
  void initParens(size_t parens) { parensAvail_ = parens; }

  void takeStates(size_t states) {
    if (states > statesAvail_)
      throw RedExceptLimit("state budget exceeded");
    statesAvail_ -= states;
  }

  void takeParens(size_t parens) {
    if (parens > parensAvail_)
      throw RedExceptLimit("nesting budget exceeded");
    parensAvail_ -= parens;
  }

  void giveStates(size_t states) { statesAvail_ += states; }
  void giveParens(size_t parens) { parensAvail_ += parens; }

private:
  size_t statesAvail_;
  size_t parensAvail_;
};

} // namespace zezax::red
