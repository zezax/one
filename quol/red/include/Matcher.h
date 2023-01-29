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


template <Length length, class InProxyT, class DfaProxyT>
Result Matcher::checkCore(InProxyT in, DfaProxyT dfap) {
  ZEZAX_RED_PREAMBLE;

  dfap.init(base, hdr->initialOff_);
  Result result = dfap.result();
  Result prevResult = 0;

  for (; in; ++in) {
    Byte byte = equivMap[*in];
    dfap.next(base, byte);
    result = dfap.result();
    if (result > 0) {
      if ((length == lenContiguous) || (length == lenLast))
        prevResult = result;
      if (length == lenShortest)
        break;
    }
    else if (((length == lenContiguous) && (prevResult > 0)) ||
             dfap.pureDeadEnd())
      break;
  }

  if ((length == lenContiguous) || (length == lenLast))
    if ((result == 0) && (prevResult > 0))
      return prevResult;

  return result;;
}


template <Length length, class InProxyT, class DfaProxyT>
Result Matcher::matchCore(InProxyT in, DfaProxyT dfap) {
  ZEZAX_RED_PREAMBLE;
  typedef typename decltype(dfap)::State State;

  dfap.init(base, hdr->initialOff_);
  const State *init = dfap.state();
  Result result = dfap.result();
  Result prevResult = 0;
  size_t idx = 0;
  size_t matchEnd = 0;

  for (; in; ++in, ++idx) {
    Byte byte = equivMap[*in];

    if (dfap.state() == init) {
      const State *prevState = dfap.state();
      dfap.next(base, byte);
      if (dfap.state() != prevState)
        matchStart_ = idx;
    }
    else
      dfap.next(base, byte);
    result = dfap.result();
    if (result > 0) {
      matchEnd = idx + 1;
      if ((length == lenContiguous) || (length == lenLast))
        prevResult = result;
      if (length == lenShortest)
        break;
    }
    else if (((length == lenContiguous) && (prevResult > 0)) ||
             dfap.pureDeadEnd())
      break;
  }

  if ((length == lenContiguous) || (length == lenLast))
    if ((result == 0) && (prevResult > 0))
      result = prevResult;

  matchEnd_ = (result == 0) ? 0 : matchEnd;
  result_ = result;
  return result;
}


template <Length length, class InProxyT, class DfaProxyT>
std::string Matcher::replaceCore(InProxyT           in,
                                 DfaProxyT          dfap,
                                 const std::string &repl) {
  ZEZAX_RED_PREAMBLE;

  dfap.init(base, hdr->initialOff_);
  std::string str;

  while (in) {
    const Byte *found = nullptr;
    DfaProxyT dproxy = dfap;
    for (InProxyT inner(in); inner; ++inner) {
      Byte byte = equivMap[*inner];
      dproxy.next(base, byte);
      if (dproxy.result() > 0)
        found = inner.ptr();
      else if (dproxy.pureDeadEnd())
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
