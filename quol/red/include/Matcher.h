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

  Result checkWhole(const void *ptr, size_t len);
  Result checkWhole(const char *str);
  Result checkWhole(const std::string &s);
  Result checkWhole(const std::string_view sv);

  Result matchLong(const void *ptr, size_t len);
  Result matchLong(const char *str);
  Result matchLong(const std::string &s);
  Result matchLong(const std::string_view sv);

private:
  template <class INPROXY, class DFAPROXY>
  Result checkWholeX(INPROXY in, DFAPROXY dfap);

  template <class INPROXY, class DFAPROXY>
  Result matchLongX(INPROXY in, DFAPROXY dfap);

  std::shared_ptr<const Executable> exec_;
  size_t                            matchStart_; // escape initial state
  size_t                            matchEnd_; // most recent accept
  Result                            result_;
  Format                            fmt_;
};

///////////////////////////////////////////////////////////////////////////////

#define ZEZAX_RED_PREAMBLE \
  const Executable *exec = exec_.get(); \
  const FileHeader *hdr = exec->getHeader(); \
  const char *base = exec->getBase(); \
  const Byte *equivMap = exec->getEquivMap()


template <class INPROXY, class DFAPROXY>
Result Matcher::checkWholeX(INPROXY in, DFAPROXY dfap) {
  ZEZAX_RED_PREAMBLE;

  const typename decltype(dfap)::State *state =
    dfap.stateAt(base, hdr->initialOff_);

  for (; in; ++in) {
    Byte byte = equivMap[*in];
    state = dfap.stateAt(base, dfap.trans(state, byte));
    if (dfap.deadEnd(state))
      break;
  }
  return dfap.result(state);
}


template <class INPROXY, class DFAPROXY>
Result Matcher::matchLongX(INPROXY in, DFAPROXY dfap) {
  ZEZAX_RED_PREAMBLE;

  size_t init = hdr->initialOff_;
  size_t off = init;
  const typename decltype(dfap)::State *state = dfap.stateAt(base, init);
  Result result = dfap.result(state);
  Result prevResult = 0;
  size_t idx = 0;

  for (; in; ++in, ++idx) {
    Byte byte = equivMap[*in];
    size_t trans = dfap.trans(state, byte);
    if ((off == init) && (off != trans))
      matchStart_ = idx;
    off = trans;
    state = dfap.stateAt(base, off);
    result = dfap.result(state);
    if (result > 0) {
      matchEnd_ = idx + 1;
      prevResult = result;
    }
    else if (dfap.pureDeadEnd(state))
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
