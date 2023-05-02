// trigger.h

#pragma once

#include <string>
#include <vector>

#include "regex.h"

namespace flume {

class TriggerT {
public:
  TriggerT(const std::string &pat, const std::vector<std::string> &args);
  ~TriggerT() { }
  bool matches(const std::string &str, MatchT &mat);
  void appendArgs(std::vector<std::string> &ref, const MatchT &mat);

protected:
  RegexT                    regex_;
  std::vector<std::string>  args_;
};

}
