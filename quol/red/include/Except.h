/* Except.h - zezax::red exception header

   These are all the custom exceptions thrown by the regular expression
   code.  All descend from std::runtime_error.  The top-level custom
   exception is RedExcept.

   Exception hierarchy:

   RedExcept              - superclass
     RedExceptInternal    - programming errors
       RedExceptCompile   - compilation errors
       RedExceptMinimize  - minimization errors
       RedExceptSerialize - serialization errors
       RedExceptExec      - execution errors
     RedExceptUser        - caused by user input
       RedExceptParse     - malformed regexes (with position)
       RedExceptApi       - bad calls/arguments
     RedExceptLimit       - limit reached
*/

#pragma once

#include <string>
#include <stdexcept>

#include "Consts.h"

namespace zezax::red {

class RedExcept : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

///////////////////////////////////////////////////////////////////////////////

class RedExceptInternal : public RedExcept {
  using RedExcept::RedExcept;
};


class RedExceptUser : public RedExcept {
  using RedExcept::RedExcept;
};


class RedExceptLimit : public RedExcept {
  using RedExcept::RedExcept;
};

///////////////////////////////////////////////////////////////////////////////

class RedExceptCompile : public RedExceptInternal {
  using RedExceptInternal::RedExceptInternal;
};


class RedExceptMinimize : public RedExceptInternal {
  using RedExceptInternal::RedExceptInternal;
};


class RedExceptSerialize : public RedExceptInternal {
  using RedExceptInternal::RedExceptInternal;
};


class RedExceptExec : public RedExceptInternal {
  using RedExceptInternal::RedExceptInternal;
};

///////////////////////////////////////////////////////////////////////////////

class RedExceptParse : public RedExceptUser {
public:
  explicit RedExceptParse(const std::string &msg)
  : RedExceptUser(msg), pos_(gNoPos) {}

  explicit RedExceptParse(const char *msg)
  : RedExceptUser(msg), pos_(gNoPos) {}

  RedExceptParse(const std::string &msg, size_t pos)
  : RedExceptUser(msg + " near position " + std::to_string(pos)),
    pos_(pos) {}

  RedExceptParse(const char *msg, size_t pos)
  : RedExceptUser(std::string(msg) + " near position " +
                       std::to_string(pos)),
    pos_(pos) {}

  RedExceptParse(const RedExceptParse &other) = default;
  RedExceptParse(RedExceptParse &&other) = default;

  virtual ~RedExceptParse() = default;

  RedExceptParse &operator=(const RedExceptParse &rhs) = default;
  RedExceptParse &operator=(RedExceptParse &&rhs) = default;

  size_t getPos() const { return pos_; }

private:
  size_t pos_;
};


class RedExceptApi : public RedExceptUser {
  using RedExceptUser::RedExceptUser;
};


} // namespace zezax::red
