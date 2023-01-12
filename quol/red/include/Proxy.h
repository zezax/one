// proxies for different input and dfa formats - header

#pragma once

#include <string>
#include <string_view>

#include "Defs.h"

namespace zezax::red {

class NullTermIter { // for C strings
public:
  //FIXME null ptr
  NullTermIter(const void *ptr) : ptr_(static_cast<const Byte *>(ptr)) {}

  Byte operator*() const { return *ptr_; }
  void operator++() { ptr_ += 1; } // !!! void
  explicit operator bool() const { return (*ptr_ != 0); }

private:
  const Byte *__restrict__ ptr_;
};


class RangeIter { // for pointer/length, std::string, std::string_view
public:
  RangeIter(const void *ptr, size_t len)
    : ptr_(static_cast<const Byte *>(ptr)), end_(ptr_ + len) {}
  RangeIter(const std::string &s)
    : ptr_(reinterpret_cast<const Byte *>(s.data())), end_(ptr_ + s.size()) {}
  RangeIter(const std::string_view &sv)
    : ptr_(reinterpret_cast<const Byte *>(sv.data())), end_(ptr_ + sv.size()) {}

  Byte operator*() const { return *ptr_; }
  void operator++() { ptr_ += 1; } // !!! void
  explicit operator bool() const { return (ptr_ < end_); }

private:
  const Byte *__restrict__ ptr_;
  const Byte *__restrict__ end_;
};

} // namespace zezax::red
