// fast matcher using serialized dfa representation - header

#pragma once

#include <memory>

#include "Exec.h"

namespace zezax::red {

enum Length {
  lenShortest   = 1,
  lenContiguous = 2,
  lenLast       = 3,
  lenWhole      = 4,
};


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

  Result checkContig(const void *ptr, size_t len);
  Result checkContig(const char *str);
  Result checkContig(const std::string &s);
  Result checkContig(const std::string_view sv);

  Result checkLast(const void *ptr, size_t len);
  Result checkLast(const char *str);
  Result checkLast(const std::string &s);
  Result checkLast(const std::string_view sv);

  Result checkWhole(const void *ptr, size_t len);
  Result checkWhole(const char *str);
  Result checkWhole(const std::string &s);
  Result checkWhole(const std::string_view sv);

  Result matchShort(const void *ptr, size_t len);
  Result matchShort(const char *str);
  Result matchShort(const std::string &s);
  Result matchShort(const std::string_view sv);

  Result matchContig(const void *ptr, size_t len);
  Result matchContig(const char *str);
  Result matchContig(const std::string &s);
  Result matchContig(const std::string_view sv);

  Result matchLast(const void *ptr, size_t len);
  Result matchLast(const char *str);
  Result matchLast(const std::string &s);
  Result matchLast(const std::string_view sv);

  Result matchWhole(const void *ptr, size_t len);
  Result matchWhole(const char *str);
  Result matchWhole(const std::string &s);
  Result matchWhole(const std::string_view sv);

  std::string replaceLast(const std::string &src, const std::string &repl);

private:
  template <Length LENGTH, class INPROXY, class DFAPROXY>
  Result checkCore(INPROXY in, DFAPROXY dfap);

  template <Length LENGTH, class INPROXY, class DFAPROXY>
  Result matchCore(INPROXY in, DFAPROXY dfap);

  template <Length LENGTH, class INPROXY, class DFAPROXY>
  std::string replaceCore(INPROXY in,
                           DFAPROXY dfap,
                           const std::string &repl);

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


template <Length LENGTH, class INPROXY, class DFAPROXY>
Result Matcher::checkCore(INPROXY in, DFAPROXY dfap) {
  ZEZAX_RED_PREAMBLE;

  const typename decltype(dfap)::State *state =
    dfap.stateAt(base, hdr->initialOff_);
  Result result = dfap.result(state);
  Result prevResult = 0;

  for (; in; ++in) {
    Byte byte = equivMap[*in];
    state = dfap.stateAt(base, dfap.trans(state, byte));
    result = dfap.result(state);
    if (result > 0) {
      if ((LENGTH == lenContiguous) || (LENGTH == lenLast))
        prevResult = result;
      if (LENGTH == lenShortest)
        break;
    }
    else if (((LENGTH == lenContiguous) && (prevResult > 0)) ||
             dfap.pureDeadEnd(state))
      break;
  }

  if ((LENGTH == lenContiguous) || (LENGTH == lenLast))
    if ((result == 0) && (prevResult > 0))
      return prevResult;

  return result;;
}


template <Length LENGTH, class INPROXY, class DFAPROXY>
Result Matcher::matchCore(INPROXY in, DFAPROXY dfap) {
  ZEZAX_RED_PREAMBLE;

  size_t init = hdr->initialOff_;
  size_t off = init;
  const typename decltype(dfap)::State *state = dfap.stateAt(base, init);
  Result result = dfap.result(state);
  Result prevResult = 0;
  size_t idx = 0;
  size_t matchEnd = 0;

  for (; in; ++in, ++idx) {
    Byte byte = equivMap[*in];
    size_t trans = dfap.trans(state, byte);
    if ((off == init) && (off != trans))
      matchStart_ = idx;
    off = trans;
    state = dfap.stateAt(base, off);
    result = dfap.result(state);
    if (result > 0) {
      matchEnd = idx + 1;
      if ((LENGTH == lenContiguous) || (LENGTH == lenLast))
        prevResult = result;
      if (LENGTH == lenShortest)
        break;
    }
    else if (((LENGTH == lenContiguous) && (prevResult > 0)) ||
             dfap.pureDeadEnd(state))
      break;
  }

  if ((LENGTH == lenContiguous) || (LENGTH == lenLast))
    if ((result == 0) && (prevResult > 0))
      result = prevResult;

  matchEnd_ = (result == 0) ? 0 : matchEnd;
  result_ = result;
  return result;
}


template <Length LENGTH, class INPROXY, class DFAPROXY>
std::string Matcher::replaceCore(INPROXY in,
                                 DFAPROXY dfap,
                                 const std::string &repl) {
  ZEZAX_RED_PREAMBLE;

  const typename decltype(dfap)::State *init =
    dfap.stateAt(base, hdr->initialOff_);

  std::string str;

  while (in) {
    const Byte *found = nullptr;
    const typename decltype(dfap)::State *state = init;
    for (INPROXY inner(in); inner; ++inner) {
      Byte byte = equivMap[*inner];
      state = dfap.stateAt(base, dfap.trans(state, byte));
      if (dfap.result(state) > 0)
        found = inner.ptr();
      else if (dfap.pureDeadEnd(state))
        break;
    }

    if (found) {
      str += repl;
      in = found + 1;
    }
    else {
      str += *in;
      ++in;
    }
  }

  return str;
}

} // namespace zezax::red
