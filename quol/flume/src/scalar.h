// scalar.h

#pragma once

#include <string>

namespace flume {

class ScalarT {
public:
  ScalarT() : isDouble_(false), u64_(0) { }

  std::string to_string() const {
    return (isDouble_ ? std::to_string(d_) : std::to_string(u64_));
  }

  void add(const std::string &s) {
    if (isDouble_)
      d_ += std::stod(s);
    else if (s.find('.') == std::string::npos)
      u64_ += std::stoull(s);
    else {
      d_ = static_cast<double>(u64_) + std::stod(s);
      isDouble_ = true;
    }
  }

protected:
  bool isDouble_;
  union {
    unsigned long long  u64_;
    double              d_;
  };
};

}
