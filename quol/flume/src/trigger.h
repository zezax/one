// trigger.h

#pragma once

#include <regex>
#include <string>
#include <vector>

namespace flume {

class TriggerT {
public:
  TriggerT(const std::string &pat, const std::vector<std::string> &args);
  ~TriggerT() { }
  bool matches(const std::string &str, std::smatch &mat);
  void appendArgs(std::vector<std::string> &ref, const std::smatch &mat);

protected:
  std::regex                regex_;
  std::vector<std::string>  args_;
};

}
