// regex exception header

#pragma once

#include <string>
#include <stdexcept>

#include "Defs.h"

namespace zezax::red {

class RedExcept : public std::runtime_error {
public:
  explicit RedExcept(const std::string &msg)
  : std::runtime_error(msg), pos_(gNoPos) {}

  explicit RedExcept(const char *msg)
  : std::runtime_error(msg), pos_(gNoPos) {}

  RedExcept(const std::string &msg, size_t pos)
  : std::runtime_error(msg + " near position " + std::to_string(pos)),
    pos_(pos) {}

  RedExcept(const char *msg, size_t pos)
  : std::runtime_error(std::string(msg) + " near position " +
                       std::to_string(pos)),
    pos_(pos) {}

  RedExcept(const RedExcept &other) = default;
  RedExcept(RedExcept &&other) = default;

  virtual ~RedExcept() = default;

  RedExcept &operator=(const RedExcept &rhs) = default;
  RedExcept &operator=(RedExcept &&rhs) = default;

  size_t getPos() const { return pos_; }

private:
  size_t pos_;
};

} // namespace zezax::red
