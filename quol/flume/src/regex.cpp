// regex.cpp

#include "regex.h"

#include <stdexcept>

using std::make_shared;
using std::runtime_error;
using std::string;

namespace flume {

string MatchT::operator[](size_t idx) const {
  string rv;
  if (src_ && (idx < size())) {
    regoff_t beg = ary_[idx].rm_so;
    regoff_t end = ary_[idx].rm_eo;
    if ((beg >= 0) && (end >= beg)) {
      size_t n = end - beg;
      rv.assign(src_ + beg, n);
    }
  }
  return rv;
}

///////////////////////////////////////////////////////////////////////////////

RegexRecT::RegexRecT(const char *pat, int cflags) {
  int err = regcomp(&prog_, pat, cflags);
  if (err) {
    char buf[1024];
    regerror(err, &prog_, buf, sizeof(buf));
    throw runtime_error(buf);
  }
}

///////////////////////////////////////////////////////////////////////////////

void RegexT::assign(const string &pat, int cflags) {
  rec_ = make_shared<RegexRecT>(pat.c_str(), cflags);
}

bool RegexT::search(const string &str, MatchT &mat) {
  int err = regexec(rec_->get(), str.c_str(), mat.size(), mat.ary_, 0);
  if (!err) {
    mat.setSrc(str.c_str());
    return true;
  }
  return false;
}

}
