// fast matcher using serialized dfa representation - header

#pragma once

#include <memory>

#include "Exec.h"
#include "Outcome.h"
#include "Proxy.h"

namespace zezax::red {

enum Style {
  styFirst      = 1,
  styContiguous = 2,
  styLast       = 3,
  styFull       = 4,
};

// all free functions; no "matcher" class

Result check(const Executable &exec, const void *ptr, size_t len, Style style);
Result check(const Executable &exec, const char *str, Style style);
Result check(const Executable &exec, const std::string &s, Style style);
Result check(const Executable &exec, std::string_view sv, Style style);

Outcome match(const Executable &exec, const void *ptr, size_t len, Style style);
Outcome match(const Executable &exec, const char *str, Style style);
Outcome match(const Executable &exec, const std::string &s, Style style);
Outcome match(const Executable &exec, std::string_view sv, Style style);

std::string replace(const Executable &exec,
                    const void       *ptr,
                    size_t            len,
                    std::string_view  repl,
                    Style             style);
std::string replace(const Executable &exec,
                    const char       *str,
                    std::string_view  repl,
                    Style             style);
std::string replace(const Executable  &exec,
                    const std::string &s,
                    std::string_view   repl,
                    Style              style);
std::string replace(const Executable &exec,
                    std::string_view  sv,
                    std::string_view  repl,
                    Style             style);

// these versions skip the run-time dispatch based on match-style
template <Style style> Result check(const Executable &exec,
                                    const void       *ptr,
                                    size_t            len);
template <Style style> Result check(const Executable &exec, const char *str);
template <Style style> Result check(const Executable  &exec,
                                    const std::string &s);
template <Style style> Result check(const Executable &exec,
                                    std::string_view  sv);

template <Style style> Outcome match(const Executable &exec,
                                     const void       *ptr,
                                     size_t            len);
template <Style style> Outcome match(const Executable &exec, const char *str);
template <Style style> Outcome match(const Executable  &exec,
                                     const std::string &s);
template <Style style> Outcome match(const Executable &exec,
                                     std::string_view  sv);

template <Style style, class InProxyT, class DfaProxyT>
Result checkCore(const Executable &exec, InProxyT in, DfaProxyT dfap);

template <Style style, class InProxyT, class DfaProxyT>
Outcome matchCore(const Executable &exec, InProxyT in, DfaProxyT dfap);

template <Style style, class InProxyT, class DfaProxyT>
std::string replaceCore(const Executable &exec,
                        InProxyT          in,
                        DfaProxyT         dfap,
                        std::string_view  repl);

///////////////////////////////////////////////////////////////////////////////

// Some macro magic here follows to define the variants of the match/check
// functions.  This reduces repetitive code and gives fewer places for
// special-case bugs to hide.

// runtime dispatch to template functions based on format
#define ZEZAX_RED_FMT_SWITCH(A_func, A_style, ...)      \
  switch (exec.getFormat()) {                           \
  case fmtDirect1: {                                    \
    DfaProxy<fmtDirect1> proxy;                         \
    return A_func<A_style>(__VA_ARGS__); }              \
  case fmtDirect2: {                                    \
    DfaProxy<fmtDirect2> proxy;                         \
    return A_func<A_style>(__VA_ARGS__); }              \
  case fmtDirect4: {                                    \
    DfaProxy<fmtDirect4> proxy;                         \
    return A_func<A_style>(__VA_ARGS__); }              \
  default:                                              \
    throw RedExceptExec("unsupported format");          \
  }


// generate template match/check functions with different prototypes
#define ZEZAX_RED_FUNC_DEFS(A_ret, A_func, ...)                          \
  template <Style style>                                                 \
  A_ret A_func(const Executable &exec, const void *ptr, size_t len) {    \
    RangeIter it(ptr, len);                                              \
    ZEZAX_RED_FMT_SWITCH(A_func ## Core, style, __VA_ARGS__)             \
  }                                                                      \
  template <Style style>                                                 \
  A_ret A_func(const Executable &exec, const char *str) {                \
    NullTermIter it(str);                                                \
    ZEZAX_RED_FMT_SWITCH(A_func ## Core, style, __VA_ARGS__)             \
  }                                                                      \
  template <Style style>                                                 \
  A_ret A_func(const Executable &exec, const std::string &s) {           \
    RangeIter it(s);                                                     \
    ZEZAX_RED_FMT_SWITCH(A_func ## Core, style, __VA_ARGS__)             \
  }                                                                      \
  template <Style style>                                                 \
  A_ret A_func(const Executable &exec, std::string_view sv) {            \
    RangeIter it(sv);                                                    \
    ZEZAX_RED_FMT_SWITCH(A_func ## Core, style, __VA_ARGS__)             \
  }


ZEZAX_RED_FUNC_DEFS(Result, check, exec, it, proxy)
ZEZAX_RED_FUNC_DEFS(Outcome, match, exec, it, proxy)

// don't #undef ZEZAX_RED_FMT_SWITCH
#undef ZEZAX_RED_FUNC_DEFS

///////////////////////////////////////////////////////////////////////////////

template <Style style, class InProxyT, class DfaProxyT>
Result checkCore(const Executable &exec, InProxyT in, DfaProxyT dfap) {
  const FileHeader *hdr = exec.getHeader();
  const char *__restrict__ base = exec.getBase();
  const Byte *__restrict__ equivMap = exec.getEquivMap();

  dfap.init(base, hdr->initialOff_);
  Result result = dfap.result();
  Result prevResult = 0;

  for (; in; ++in) {
    Byte byte = equivMap[*in];
    dfap.next(base, byte);
    result = dfap.result();
    if (UNLIKELY(result > 0)) {
      if ((style == styContiguous) || (style == styLast))
        prevResult = result;
      if (style == styFirst)
        break;
    }
    else if (((style == styContiguous) && (prevResult > 0)) ||
             dfap.pureDeadEnd())
      break;
  }

  if ((style == styContiguous) || (style == styLast))
    if ((result == 0) && (prevResult > 0))
      return prevResult;

  return result;;
}


template <Style style, class InProxyT, class DfaProxyT>
Outcome matchCore(const Executable &exec, InProxyT in, DfaProxyT dfap) {
  typedef typename decltype(dfap)::State State;

  const FileHeader *hdr = exec.getHeader();
  const char *__restrict__ base = exec.getBase();
  const Byte *__restrict__ equivMap = exec.getEquivMap();

  dfap.init(base, hdr->initialOff_);
  const State *__restrict__ init = dfap.state();
  Result result = dfap.result();
  Result prevResult = 0;
  size_t idx = 0;
  size_t matchStart = 0;
  size_t matchEnd = 0;

  for (; in; ++in, ++idx) {
    Byte byte = equivMap[*in];

    if (UNLIKELY(dfap.state() == init)) {
      const State *__restrict__ prevState = dfap.state();
      dfap.next(base, byte);
      if (dfap.state() != prevState)
        matchStart = idx;
    }
    else
      dfap.next(base, byte);
    result = dfap.result();
    if (UNLIKELY(result > 0)) {
      matchEnd = idx + 1;
      if ((style == styContiguous) || (style == styLast))
        prevResult = result;
      if (style == styFirst)
        break;
    }
    else if (((style == styContiguous) && (prevResult > 0)) ||
             dfap.pureDeadEnd())
      break;
  }

  if ((style == styContiguous) || (style == styLast))
    if ((result == 0) && (prevResult > 0))
      result = prevResult;

  Outcome rv;
  rv.result_ = result;
  if (result == 0) {
    rv.start_ = 0;
    rv.end_   = 0;
  }
  else {
    rv.start_ = matchStart;
    rv.end_   = matchEnd;
  }
  return rv;
}


template <Style style, class InProxyT, class DfaProxyT>
std::string replaceCore(const Executable &exec,
                        InProxyT          in,
                        DfaProxyT         dfap,
                        std::string_view  repl) {
  const FileHeader *hdr = exec.getHeader();
  const char *__restrict__ base = exec.getBase();
  const Byte *__restrict__ equivMap = exec.getEquivMap();

  dfap.init(base, hdr->initialOff_);
  std::string str;

  while (in) {
    const Byte *__restrict__ found = nullptr;
    DfaProxyT dproxy = dfap;
    for (InProxyT inner(in); inner; ++inner) {
      Byte byte = equivMap[*inner];
      dproxy.next(base, byte);
      if (UNLIKELY(dproxy.result() > 0)) {
        found = inner.ptr();
        if (style == styFirst)
          break;
      }
      else {
        if (style == styFull)
          found = nullptr;
        if (((style == styContiguous) && found) ||
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
