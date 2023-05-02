// regex.h

#pragma once

#include <sys/types.h>
#include <regex.h>

#include <memory>
#include <string>

namespace flume {

class MatchT {
public:
  MatchT() : src_(nullptr) { }

  size_t size() const { return sizeof(ary_) / sizeof(ary_[0]); }
  std::string operator[](size_t idx) const;

private:
  void setSrc(const char *src) { src_ = src; }

  const char  *src_;
  regmatch_t   ary_[10];

  friend class RegexT;
};

class RegexRecT {
public:
  RegexRecT(const char *pat, int cflags);
  ~RegexRecT() { regfree(&prog_); }

  // no copying or moving
  RegexRecT(const RegexRecT &other) = delete;
  RegexRecT &operator=(const RegexRecT &rhs) = delete;

  const regex_t *get() const { return &prog_; }

private:
  regex_t  prog_;
};

class RegexT {
public:
  void assign(const std::string &pat, int cflags);
  bool search(const std::string &str, MatchT &mat);

private:
  std::shared_ptr<RegexRecT>  rec_;
};
  
}
