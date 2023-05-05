// util.cpp - utility code

#include "util.h"

#include <sys/stat.h>

#include <string>

namespace zezax::sensorrd {

using std::string;
using std::string_view;


bool startsWith(string_view str, string_view head) {
  if (str.size() < head.size())
    return false;
  string_view pfx(str.data(), head.size());
  return (pfx == head);
}


bool endsWith(string_view str, string_view tail) {
  if (str.size() < tail.size())
    return false;
  string_view suf(str.data() + (str.size() - tail.size()), tail.size());
  return (suf == tail);
}


void split(string_view  src,
           char         delim,
           string_view &pfx,
           string_view &suf) {
  size_t pos = src.find(delim);
  if (pos == string_view::npos)
    pos = src.size();
  pfx = string_view(src.data(), pos);
  suf = string_view(src.data() + pos, src.size() - pos);
}


bool pathExists(const string &path) {
  struct stat sst;
  int res = stat(path.c_str(), &sst);
  return (res == 0);
}

} // namespace zezax::sensorrd
