// outcome or result of a match - header

#pragma once

#include "Types.h"

namespace zezax::red {

/*
  A little explanation...

  Result = a number indicating which match happened, if any.  Zero means no
           match was found.  Otherwise, the number is the same as was passed
           to Parser::add() or Parser::addAuto().  If no differentiation is
           needed, 1 can be used for all patterns.

  Outcome = a structure containing a Result and also some location information
            related to the match.

  The 'check' functions return a Result.  The 'match' functions return an
  Outcome.

  start_ is the position within the input where the DFA "escaped" its
  initial state.  This is not foolproof, but typically works well if the
  pattern is something like '.*[0-9]+', in which case start_ would indicate
  the first digit.

  end_ is the last position at which the DFA was in an accepting state.
  This is quite reliable (unlike start_).  It will depend on the match style,
  though.  Using styFirst, styTangent or styLast makes the most sense for this.
*/

struct Outcome {
  Result result_;
  size_t start_;
  size_t end_;

  bool operator==(const Outcome &rhs) const {
    return ((result_ == rhs.result_) &&
            (start_ == rhs.start_) && (end_ == rhs.end_));
  }

  explicit operator bool() const { return (result_ > 0); }

  static Outcome fail() {
    Outcome rv;
    rv.result_ = 0;
    rv.start_ = 0;
    rv.end_= 0 ;
    return rv;
  }
};

} // namespace zezax::red
