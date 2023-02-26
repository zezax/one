// fast matcher using serialized dfa representation - header

#pragma once

#include <memory>

#include "Exec.h"
#include "Proxy.h"

namespace zezax::red {

enum Length {
  lenShortest   = 1,
  lenContiguous = 2,
  lenLast       = 3,
  lenFull       = 4,
};


class Matcher {
public:
  explicit Matcher(std::shared_ptr<const Executable> exec);
  explicit Matcher(const Executable *exec); // to avoid shared_ptr overhead

  explicit operator bool() const { return (result_ > 0); }

  void reset();

  Result result() const { return result_; }
  size_t start() const { return matchStart_; }
  size_t end() const { return matchEnd_; }

  Result check(const void *ptr, size_t len, Length mlen = lenFull);
  Result check(const char *str, Length mlen = lenFull);
  Result check(const std::string &s, Length mlen = lenFull);
  Result check(std::string_view sv, Length mlen = lenFull);

  Result match(const void *ptr, size_t len, Length mlen = lenFull);
  Result match(const char *str, Length mlen = lenFull);
  Result match(const std::string &s, Length mlen = lenFull);
  Result match(std::string_view sv, Length mlen = lenFull);

  std::string replace(const void       *ptr,
                      size_t            len,
                      std::string_view  repl,
                      Length            mlen = lenFull);
  std::string replace(const char       *str,
                      std::string_view  repl,
                      Length            mlen = lenFull);
  std::string replace(const std::string &s,
                      std::string_view   repl,
                      Length             mlen = lenFull);
  std::string replace(std::string_view  sv,
                      std::string_view  repl,
                      Length            mlen = lenFull);

  // these versions skip the run-time dispatch based on match-length
  template <Length length> Result check(const void *ptr, size_t len);
  template <Length length> Result check(const char *str);
  template <Length length> Result check(const std::string &s);
  template <Length length> Result check(std::string_view sv);

  template <Length length> Result match(const void *ptr, size_t len);
  template <Length length> Result match(const char *str);
  template <Length length> Result match(const std::string &s);
  template <Length length> Result match(std::string_view sv);

private:
  template <Length LENGTH, class INPROXY, class DFAPROXY>
  Result checkCore(INPROXY in, DFAPROXY dfap);

  template <Length LENGTH, class INPROXY, class DFAPROXY>
  Result matchCore(INPROXY in, DFAPROXY dfap);

  template <Length LENGTH, class INPROXY, class DFAPROXY>
  std::string replaceCore(INPROXY          in,
                          DFAPROXY         dfap,
                          std::string_view repl);

  const Executable                  *exec_;
  size_t                             matchStart_; // escape initial state
  size_t                             matchEnd_;   // most recent accept
  Result                             result_;
  Format                             fmt_;
  std::shared_ptr<const Executable>  shared_;     // only if supplied
};

///////////////////////////////////////////////////////////////////////////////

// Some macro magic here follows to define the variants of the match/check
// functions.  This reduces repetitive code and gives fewer places for
// special-case bugs to hide.

// runtime dispatch to template functions based on format
#define ZEZAX_RED_FMT_SWITCH(A_func, A_len, ...)        \
  switch (fmt_) {                                       \
  case fmtDirect1: {                                    \
    DfaProxy<fmtDirect1> proxy;                         \
    return A_func<A_len>(__VA_ARGS__); }                \
  case fmtDirect2: {                                    \
    DfaProxy<fmtDirect2> proxy;                         \
    return A_func<A_len>(__VA_ARGS__); }                \
  case fmtDirect4: {                                    \
    DfaProxy<fmtDirect4> proxy;                         \
    return A_func<A_len>(__VA_ARGS__); }                \
  default:                                              \
    throw RedExceptExec("unsupported format");          \
  }


// generate template match/check functions with different prototypes
#define ZEZAX_RED_FUNC_DEFS(A_func, ...)                        \
  template <Length length>                                      \
  Result Matcher::A_func(const void *ptr, size_t len) {         \
    RangeIter it(ptr, len);                                     \
    ZEZAX_RED_FMT_SWITCH(A_func ## Core, length, __VA_ARGS__)   \
  }                                                             \
  template <Length length>                                      \
  Result Matcher::A_func(const char *str) {                     \
    NullTermIter it(str);                                       \
    ZEZAX_RED_FMT_SWITCH(A_func ## Core, length, __VA_ARGS__)   \
  }                                                             \
  template <Length length>                                      \
  Result Matcher::A_func(const std::string &s) {                \
    RangeIter it(s);                                            \
    ZEZAX_RED_FMT_SWITCH(A_func ## Core, length, __VA_ARGS__)   \
  }                                                             \
  template <Length length>                                      \
  Result Matcher::A_func(std::string_view sv) {                 \
    RangeIter it(sv);                                           \
    ZEZAX_RED_FMT_SWITCH(A_func ## Core, length, __VA_ARGS__)   \
  }


ZEZAX_RED_FUNC_DEFS(check, it, proxy)
ZEZAX_RED_FUNC_DEFS(match, it, proxy)

// don't #undef ZEZAX_RED_FMT_SWITCH
#undef ZEZAX_RED_FUNC_DEFS

///////////////////////////////////////////////////////////////////////////////

template <Length length, class InProxyT, class DfaProxyT>
Result Matcher::checkCore(InProxyT in, DfaProxyT dfap) {
  const FileHeader *hdr = exec_->getHeader();
  const char *base = exec_->getBase();
  const Byte *equivMap = exec_->getEquivMap();

  dfap.init(base, hdr->initialOff_);
  Result result = dfap.result();
  Result prevResult = 0;

  for (; in; ++in) {
    Byte byte = equivMap[*in];
    dfap.next(base, byte);
    result = dfap.result();
    if (UNLIKELY(result > 0)) {
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
  typedef typename decltype(dfap)::State State;

  const FileHeader *hdr = exec_->getHeader();
  const char *base = exec_->getBase();
  const Byte *equivMap = exec_->getEquivMap();

  dfap.init(base, hdr->initialOff_);
  const State *init = dfap.state();
  Result result = dfap.result();
  Result prevResult = 0;
  size_t idx = 0;
  size_t matchEnd = 0;

  for (; in; ++in, ++idx) {
    Byte byte = equivMap[*in];

    if (UNLIKELY(dfap.state() == init)) {
      const State *prevState = dfap.state();
      dfap.next(base, byte);
      if (dfap.state() != prevState)
        matchStart_ = idx;
    }
    else
      dfap.next(base, byte);
    result = dfap.result();
    if (UNLIKELY(result > 0)) {
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
std::string Matcher::replaceCore(InProxyT         in,
                                 DfaProxyT        dfap,
                                 std::string_view repl) {
  const FileHeader *hdr = exec_->getHeader();
  const char *base = exec_->getBase();
  const Byte *equivMap = exec_->getEquivMap();

  dfap.init(base, hdr->initialOff_);
  std::string str;

  while (in) {
    const Byte *found = nullptr;
    DfaProxyT dproxy = dfap;
    for (InProxyT inner(in); inner; ++inner) {
      Byte byte = equivMap[*inner];
      dproxy.next(base, byte);
      if (UNLIKELY(dproxy.result() > 0)) {
        found = inner.ptr();
        if (length == lenShortest)
          break;
      }
      else {
        if (length == lenFull)
          found = nullptr;
        if (((length == lenContiguous) && found) ||
            dproxy.pureDeadEnd())
          break;
      }
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
