// proxies for different input and dfa formats - header

#pragma once

#include <string>
#include <string_view>

#include "Types.h"
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

  // less safe stuff for replaceCore()
  const Byte *ptr() const { return ptr_; }
  void operator=(const Byte *p) { ptr_ = p; }

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

template <Format fmt>
struct DfaDefs {};

template <>
struct DfaDefs<fmtDirect1> {
  typedef StateDirect1 State;
  typedef uint8_t      Value;

  static constexpr Value  resultMask_   = 0x7f;
  static constexpr Result maxResult_    = 0x7f;
  static constexpr size_t maxOffset_    = 0xff;
  static constexpr int    deadEndShift_ = 7;
};


template <>
struct DfaDefs<fmtDirect2> {
  typedef StateDirect2 State;
  typedef uint16_t     Value;

  static constexpr Value  resultMask_   = 0x7fff;
  static constexpr Result maxResult_    = 0x7fff;
  static constexpr size_t maxOffset_    = 0xffff;
  static constexpr int    deadEndShift_ = 15;
};


template <>
struct DfaDefs<fmtDirect4> {
  typedef StateDirect4 State;
  typedef uint32_t     Value;

  static constexpr Value  resultMask_   = 0x7fffffff;
  static constexpr Result maxResult_    = 0x7fffffff;
  static constexpr size_t maxOffset_    = 0xffffffff;
  static constexpr int    deadEndShift_ = 31;
};


template <Format fmt>
class DfaProxy {
public:
  typedef typename DfaDefs<fmt>::State State;
  typedef typename DfaDefs<fmt>::Value Value;

  static constexpr Value  resultMask_   = DfaDefs<fmt>::resultMask_;
  static constexpr Result maxResult_    = DfaDefs<fmt>::maxResult_;
  static constexpr size_t maxOffset_    = DfaDefs<fmt>::maxOffset_;
  static constexpr int    deadEndShift_ = DfaDefs<fmt>::deadEndShift_;

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
      throw RedExceptLimit("DFA too big for format");
  }

  static size_t stateSize(CharIdx maxChar) {
    return sizeof(State) + (sizeof(Value) * maxChar) + sizeof(Value);
  }

  static void appendOff(std::string &buf, size_t off) {
    off /= sizeof(Value);
    if (off > maxOffset_)
      throw RedExceptSerialize("overflow in appendOff");
    Value raw = static_cast<Value>(off);
    buf.append(reinterpret_cast<const char *>(&raw), sizeof(raw));
  }

  static Value resultAndDeadEnd(Result res, bool de) {
    return (res & resultMask_) | (de << deadEndShift_);
  }

  Result result() const {
    return state_->resultAndDeadEnd_ & resultMask_;
  }

  bool deadEnd() const {
    return state_->resultAndDeadEnd_ >> deadEndShift_;
  }

  bool pureDeadEnd() const {
    return (state_->resultAndDeadEnd_ == (1u << deadEndShift_));
  }

  void init(const char *base, size_t offset) {
    state_ = reinterpret_cast<const State *>(base + offset);
  }

  size_t trans(size_t byte) const {
    return state_->offsets_[byte] * sizeof(Value);
  }

  void next(const char *base, size_t byte) { init(base, trans(byte)); }

  const State *state() const { return state_; }

private:
  const State *__restrict__ state_;
};

} // namespace zezax::red
