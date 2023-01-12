// fast matcher using serialized dfa representation - header

#pragma once

#include <memory>

#include "Exec.h"

namespace zezax::red {

class Matcher {
public:
  Matcher(std::shared_ptr<const Executable> exec);

  explicit operator bool() const { return (result_ > 0); }

  void reset();

  Result result() const { return result_; }
  size_t start() const { return matchStart_; }
  size_t end() const { return matchEnd_; }

  Result checkShort(const void *ptr, size_t len);
  Result checkShort(const char *str);
  Result checkShort(const std::string &s);
  Result checkShort(const std::string_view sv);

  Result matchLong(const void *ptr, size_t len);
  Result matchLong(const char *str);
  Result matchLong(const std::string &s);
  Result matchLong(const std::string_view sv);

private:
  template <class INPROXY>
  Result checkShort4(INPROXY in);

  template <class INPROXY>
  Result matchLong4(INPROXY in);

  std::shared_ptr<const Executable> exec_;
  size_t                            matchStart_; // escape initial state
  size_t                            matchEnd_; // most recent accept
  Result                            result_;
};

///////////////////////////////////////////////////////////////////////////////

template <class INPROXY>
Result Matcher::checkShort4(INPROXY in) {
  const FileHeader *hdr = exec_->getHeader();
  const char *base = exec_->getBase();
  const Byte *equivMap = exec_->getEquivMap();

  const StateOffset4 *state =
    reinterpret_cast<const StateOffset4 *>(base + hdr->initialOff_);
  for (; in; ++in) {
    Byte byte = equivMap[*in];
    state = reinterpret_cast<const StateOffset4 *>(
            base + (state->offsets_[byte] << 2));
    uint32_t resultAndDeadEnd = state->resultAndDeadEnd_;
    if (resultAndDeadEnd) // either accept or dead end
      return resultAndDeadEnd & 0x7fffffff;
  }
  return state->resultAndDeadEnd_ & 0x7fffffff;
}


template <class INPROXY>
Result Matcher::matchLong4(INPROXY in) {
  const FileHeader *hdr = exec_->getHeader();
  const char *base = exec_->getBase();
  const Byte *equivMap = exec_->getEquivMap();

  size_t init = hdr->initialOff_;
  size_t off = init;
  const StateOffset4 *state =
    reinterpret_cast<const StateOffset4 *>(base + off);
  Result result = state->resultAndDeadEnd_ & 0x7fffffff;
  Result prevResult = 0;
  size_t idx = 0;

  for (; in; ++in, ++idx) {
    Byte byte = equivMap[*in];
    size_t trans = state->offsets_[byte] << 2;
    if ((off == init) && (off != trans))
      matchStart_ = idx;
    off = trans;
    state = reinterpret_cast<const StateOffset4 *>(base + off);
    result = state->resultAndDeadEnd_ & 0x7fffffff;
    if (result > 0) {
      matchEnd_ = idx + 1;
      prevResult = result;
    }
    else if (state->resultAndDeadEnd_ == 0x80000000) // non-accept dead end
      break;
  }

  if ((result == 0) && (prevResult > 0)) {
    result_ = prevResult;
    return prevResult;
  }

  result_ = result;
  return result;
}

} // namespace zezax::red
