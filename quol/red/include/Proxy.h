// proxies for different input and dfa formats - header

#pragma once

#include <string>
#include <string_view>

#include "Defs.h"
#include "Except.h"
#include "Serializer.h"

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

  // less safe stuff for replaceCore()
  const Byte *ptr() const { return ptr_; }
  void operator=(const Byte *p) { ptr_ = p; }

private:
  const Byte *__restrict__ ptr_;
  const Byte *__restrict__ end_;
};

///////////////////////////////////////////////////////////////////////////////

// FIXME: make State * state_ member, next() method

template <Format FMT>
class DfaProxy {};

template <>
class DfaProxy<fmtOffset1> {
public:
  typedef StateOffset1 State;
  typedef uint8_t Value;

  static constexpr Value  resultMask_   = 0x7f;
  static constexpr Result maxResult_    = 0x7f;
  static constexpr size_t maxOffset_    = 0xff;
  static constexpr int    deadEndShift_ = 7;

  static bool resultFits(Result maxResult) {
    return (maxResult <= maxResult_);
  }

  static bool offsetFits(CharIdx maxChar, size_t numStates) {
    // !!! assume result is same size as transition entries
    size_t tot = (numStates * maxChar) + numStates + numStates;
    return (tot <= maxOffset_);
  }

  static void checkCapacity(size_t  numStates,
                            CharIdx maxChar,
                            Result  maxResult) {
    if (!resultFits(maxResult) || !offsetFits(maxChar, numStates))
      throw RedExceptLimit("DFA too big for 1-byte format");
  }

  static size_t stateSize(CharIdx maxChar) {
    return sizeof(State) + (sizeof(Value) * maxChar) + sizeof(Value);
  }

  static void appendOff(std::string &buf, size_t off) {
    if (off > maxOffset_)
      throw RedExceptSerialize("overflow in 1-byte appendOff");
    Value raw = static_cast<Value>(off);
    buf.append(reinterpret_cast<const char *>(&raw), sizeof(raw));
  }

  static const State *stateAt(const char *base, size_t off) {
    return reinterpret_cast<const State *>(base + off);
  }

  static size_t trans(const State *state, size_t byte) {
    return state->offsets_[byte];
  }

  static uint8_t resultAndDeadEnd(Result res, bool de) {
    return (res & resultMask_) | (de << deadEndShift_);
  }

  static Result result(const State *state) {
    return state->resultAndDeadEnd_ & resultMask_;
  }

  static bool deadEnd(const State *state) {
    return state->resultAndDeadEnd_ >> deadEndShift_;
  }

  static bool pureDeadEnd(const State *state) {
    return (state->resultAndDeadEnd_ == (1u << deadEndShift_));
  }
};


template <>
class DfaProxy<fmtOffset2> {
public:
  typedef StateOffset2 State;
  typedef uint16_t Value;

  static constexpr Value  resultMask_   = 0x7fff;
  static constexpr Result maxResult_    = 0x7fff;
  static constexpr size_t maxOffset_    = 0xffff;
  static constexpr int    deadEndShift_ = 15;

  static bool resultFits(Result maxResult) {
    return (maxResult <= maxResult_);
  }

  static bool offsetFits(CharIdx maxChar, size_t numStates) {
    // !!! assume result is same size as transition entries
    size_t tot = (numStates * maxChar) + numStates + numStates;
    return (tot <= maxOffset_);
  }

  static void checkCapacity(size_t  numStates,
                            CharIdx maxChar,
                            Result  maxResult) {
    if (!resultFits(maxResult) || !offsetFits(maxChar, numStates))
      throw RedExceptLimit("DFA too big for 2-byte format");
  }

  static size_t stateSize(CharIdx maxChar) {
    return sizeof(State) + (sizeof(Value) * maxChar) + sizeof(Value);
  }

  static void appendOff(std::string &buf, size_t off) {
    off /= sizeof(Value);
    if (off > maxOffset_)
      throw RedExceptSerialize("overflow in 2-byte appendOff");
    Value raw = static_cast<Value>(off);
    buf.append(reinterpret_cast<const char *>(&raw), sizeof(raw));
  }

  static const State *stateAt(const char *base, size_t off) {
    return reinterpret_cast<const State *>(base + off);
  }

  static size_t trans(const State *state, size_t byte) {
    return state->offsets_[byte] * sizeof(Value);
  }

  static uint16_t resultAndDeadEnd(Result res, bool de) {
    return (res & resultMask_) | (de << deadEndShift_);
  }

  static Result result(const State *state) {
    return state->resultAndDeadEnd_ & resultMask_;
  }

  static bool deadEnd(const State *state) {
    return state->resultAndDeadEnd_ >> deadEndShift_;
  }

  static bool pureDeadEnd(const State *state) {
    return (state->resultAndDeadEnd_ == (1u << deadEndShift_));
  }
};


template <>
class DfaProxy<fmtOffset4> {
public:
  typedef StateOffset4 State;
  typedef uint32_t Value;

  static constexpr Value  resultMask_   = 0x7fffffff;
  static constexpr Result maxResult_    = 0x7fffffff;
  static constexpr size_t maxOffset_    = 0xffffffff;
  static constexpr int    deadEndShift_ = 31;

  static bool resultFits(Result maxResult) {
    return (maxResult <= maxResult_);
  }

  static bool offsetFits(CharIdx maxChar, size_t numStates) {
    // !!! assume result is same size as transition entries
    size_t tot = (numStates * maxChar) + numStates + numStates;
    return (tot <= maxOffset_);
  }

  static void checkCapacity(size_t  numStates,
                            CharIdx maxChar,
                            Result  maxResult) {
    if (!resultFits(maxResult) || !offsetFits(maxChar, numStates))
      throw RedExceptLimit("DFA too big for 4-byte format");
  }

  static size_t stateSize(CharIdx maxChar) {
    return sizeof(State) + (sizeof(Value) * maxChar) + sizeof(Value);
  }

  static void appendOff(std::string &buf, size_t off) {
    off /= sizeof(Value);
    if (off > maxOffset_)
      throw RedExceptSerialize("overflow in 4-byte appendOff");
    Value raw = static_cast<Value>(off);
    buf.append(reinterpret_cast<const char *>(&raw), sizeof(raw));
  }

  static const State *stateAt(const char *base, size_t off) {
    return reinterpret_cast<const State *>(base + off);
  }

  static size_t trans(const State *state, size_t byte) {
    return state->offsets_[byte] * sizeof(Value);
  }

  static uint32_t resultAndDeadEnd(Result res, bool de) {
    return (res & resultMask_) | (de << deadEndShift_);
  }

  static Result result(const State *state) {
    return state->resultAndDeadEnd_ & resultMask_;
  }

  static bool deadEnd(const State *state) {
    return state->resultAndDeadEnd_ >> deadEndShift_;
  }

  static bool pureDeadEnd(const State *state) {
    return (state->resultAndDeadEnd_ == (1u << deadEndShift_));
  }
};

} // namespace zezax::red
