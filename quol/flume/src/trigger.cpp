// trigger.cpp

#include "trigger.h"

using std::string;
using std::vector;

namespace flume {

TriggerT::TriggerT(const string &pat, const vector<string> &args)
  : args_(args)
{
  if (pat.find("(?i)") == 0) {
    regex_.assign(pat.c_str() + 4, std::regex::icase);;
  }
  else {
    regex_ = pat;
  }
}


bool
TriggerT::matches(const string &str, std::smatch &mat)
{
  return std::regex_search(str, mat, regex_);
}


void
TriggerT::appendArgs(vector<string> &ref, const std::smatch &mat)
{
  size_t nmat = mat.size();
  if (nmat > 10)
    nmat = 10;

  for (const string &arg : args_) {
    if (arg.find('\\') == string::npos)
      ref.push_back(arg);
    else {                      // backreference substitution
      string buf;
      const char *ptr = arg.c_str();
      const char *end = ptr + arg.size();
      for (; ptr < end; ++ptr) {
        char ch = ptr[0];
        if (ch == '\\') {
          int idx = ptr[1] - '0';
          if ((idx >= 0) && (static_cast<size_t>(idx) < nmat)) {
            buf += mat[idx];
            ++ptr;
          }
          else if (ptr[1] == '*') {
            buf += ref[0];      // whole line
            ++ptr;
          }
          else
            buf += '\\';
        }
        else
          buf += ch;
      }
      ref.emplace_back(std::move(buf));
    }
  }
}

}
