// fast matcher using serialized dfa representation - header

#pragma once

#include <memory>

#include "Executable.h"
#include "Outcome.h"
#include "Proxy.h"

namespace zezax::red {

/*
  There is no "matcher class", just free functions and templates.

  Variations are provided based on a number of axes:

  VERB: check   - lightweight, archored to front of text, returns Result
        match   - like check, but returns full Outcome, slightly slower
        scan    - sliding-window version of check; finds first match
        search  - sliding-window version of match; finds first match
        replace - rewrite string based on regex match(es)

  INPUT: void pointer and length
         char pointer (C-style null-terminated)
         std::string
         std::string_view

  STYLE: when to finish matching, based on accepting states of DFA (see below)

  DO-LEADER: true  - optimize matching based on required prefix if present
             false - disable optimization, for known-leaderless cases

  Styles Described:

  Instant: stop at shortest possible match
           re('[0-9]+' -> 1) on '0123456789' matches '0'
           re('abc' -> 1, 'abcd' -> 2) on 'abcde' matches 'abc'
           re('new' -> 1, 'new york' -> 2) on 'new york' matches 'new'
  First:   shortest match and then continue while result is the same
           re('[0-9]+' -> 1) on '0123456789' matches '0123456789'
           re('abc' -> 1, 'abcd' -> 2) on 'abcde' matches 'abc'
           re('new' -> 1, 'new york' -> 2) on 'new york' matches 'new'
  Tangent: shortest match and continue while result is accepting
           re('[0-9]+' -> 1) on '0123456789' matches '0123456789'
           re('abc' -> 1, 'abcd' -> 2) on 'abcde' matches 'abcd'
           re('new' -> 1, 'new york' -> 2) on 'new york' matches 'new'
  Last:    longest accepting match
           re('[0-9]+' -> 1) on '0123456789' matches '0123456789'
           re('abc' -> 1, 'abcd' -> 2) on 'abcde' matches 'abcd'
           re('new' -> 1, 'new york' -> 2) on 'new york' matches 'new york'
  Full:    entire text must match
           re('[0-9]+' -> 1) on '0123456789' matches '0123456789'
           re('abc' -> 1, 'abcd' -> 2) on 'abcde' fails
           re('new' -> 1, 'new york' -> 2) on 'new york' matches 'new york'
 */

enum Style {
  styInvalid = 0,
  styInstant = 1,
  styFirst   = 2,
  styTangent = 3,
  styLast    = 4,
  styFull    = 5,
};


// these variants allow style to be set at run-time, default to do leader opt

Result check(const Executable &exec, const void *ptr, size_t len, Style style);
Result check(const Executable &exec, const char *str, Style style);
Result check(const Executable &exec, const std::string &s, Style style);
Result check(const Executable &exec, std::string_view sv, Style style);

Outcome match(const Executable &exec, const void *ptr, size_t len, Style style);
Outcome match(const Executable &exec, const char *str, Style style);
Outcome match(const Executable &exec, const std::string &s, Style style);
Outcome match(const Executable &exec, std::string_view sv, Style style);

Result scan(const Executable &exec, const void *ptr, size_t len, Style style);
Result scan(const Executable &exec, const char *str, Style style);
Result scan(const Executable &exec, const std::string &s, Style style);
Result scan(const Executable &exec, std::string_view sv, Style style);

Outcome search(const Executable &exec, const void *ptr, size_t len,
               Style style);
Outcome search(const Executable &exec, const char *str, Style style);
Outcome search(const Executable &exec, const std::string &s, Style style);
Outcome search(const Executable &exec, std::string_view sv, Style style);

size_t replace(const Executable &exec,
               const void       *ptr,
               size_t            len,
               std::string_view  repl,
               std::string      &out,
               size_t            max,
               Style             style);
size_t replace(const Executable &exec,
               const char       *str,
               std::string_view  repl,
               std::string      &out,
               size_t            max,
               Style             style);
size_t replace(const Executable  &exec,
               const std::string &s,
               std::string_view   repl,
               std::string       &out,
               size_t             max,
               Style              style);
size_t replace(const Executable &exec,
               std::string_view  sv,
               std::string_view  repl,
               std::string      &out,
               size_t            max,
               Style             style);

// this exists as a workalike for RE2::Set::Match()
size_t matchAll(const Executable     &exec,
                std::string_view      sv,
                std::vector<Outcome> &out);

// the following variants skip the run-time dispatch based on style

template <Style style, bool doLeader>
Result check(const Executable &exec, const void *ptr, size_t len);

template <Style style, bool doLeader>
Result check(const Executable &exec, const char *str);

template <Style style, bool doLeader>
Result check(const Executable &exec, const std::string &s);

template <Style style, bool doLeader>
Result check(const Executable &exec, std::string_view  sv);


template <Style style, bool doLeader>
Outcome match(const Executable &exec, const void *ptr, size_t len);

template <Style style, bool doLeader>
Outcome match(const Executable &exec, const char *str);

template <Style style, bool doLeader>
Outcome match(const Executable &exec, const std::string &s);

template <Style style, bool doLeader>
Outcome match(const Executable &exec, std::string_view  sv);


template <Style style, bool doLeader>
Result scan(const Executable &exec, const void *ptr, size_t len);

template <Style style, bool doLeader>
Result scan(const Executable &exec, const char *str);

template <Style style, bool doLeader>
Result scan(const Executable &exec, const std::string &s);

template <Style style, bool doLeader>
Result scan(const Executable &exec, std::string_view  sv);


template <Style style, bool doLeader>
Outcome search(const Executable &exec, const void *ptr, size_t len);

template <Style style, bool doLeader>
Outcome search(const Executable &exec, const char *str);

template <Style style, bool doLeader>
Outcome search(const Executable &exec, const std::string &s);

template <Style style, bool doLeader>
Outcome search(const Executable &exec, std::string_view  sv);


template <Style style, bool doLeader>
size_t replace(const Executable &exec,
               const void       *ptr,
               size_t            len,
               std::string_view  repl,
               std::string      &out,
               size_t            max);

template <Style style, bool doLeader>
size_t replace(const Executable &exec,
               const char       *str,
               std::string_view  repl,
               std::string      &out,
               size_t            max);

template <Style style, bool doLeader>
size_t replace(const Executable  &exec,
               const std::string &s,
               std::string_view   repl,
               std::string       &out,
               size_t             max);

template <Style style, bool doLeader>
size_t replace(const Executable &exec,
               std::string_view  sv,
               std::string_view  repl,
               std::string      &out,
               size_t            max);

// these are the actual core templates

template <Style style, bool doLeader, class InProxyT, class DfaProxyT>
Result checkCore(const Executable &exec, InProxyT in, DfaProxyT dfap);

template <Style style, bool doLeader, class InProxyT, class DfaProxyT>
Outcome matchCore(const Executable &exec, InProxyT in, DfaProxyT dfap);

template <Style style, bool doLeader, class InProxyT, class DfaProxyT>
Result scanCore(const Executable &exec, InProxyT in, DfaProxyT dfap);

template <Style style, bool doLeader, class InProxyT, class DfaProxyT>
Outcome searchCore(const Executable &exec, InProxyT in, DfaProxyT dfap);

template <Style style, bool doLeader, class InProxyT, class DfaProxyT>
size_t replaceCore(const Executable &exec,
                   InProxyT          in,
                   DfaProxyT         dfap,
                   std::string_view  repl,
                   std::string      &out,
                   size_t            max);

template <Style style, bool doLeader, class InProxyT, class DfaProxyT>
size_t matchAllCore(const Executable     &exec, // a la RE2::Set::Match()
                    InProxyT              in,
                    DfaProxyT             dfap,
                    std::vector<Outcome> &out);

///////////////////////////////////////////////////////////////////////////////

// Some macro magic here follows to define the variants of the primary
// functions.  This reduces repetitive code and gives fewer places for
// special-case bugs to hide.

// runtime dispatch to template functions based on format
#define ZEZAX_RED_FMT_SWITCH(A_func, A_style, A_lead, ...) \
  switch (exec.getFormat()) {                              \
  case fmtDirect1: {                                       \
    DfaProxy<fmtDirect1> proxy;                            \
    return A_func<A_style, A_lead>(__VA_ARGS__); }         \
  case fmtDirect2: {                                       \
    DfaProxy<fmtDirect2> proxy;                            \
    return A_func<A_style, A_lead>(__VA_ARGS__); }         \
  case fmtDirect4: {                                       \
    DfaProxy<fmtDirect4> proxy;                            \
    return A_func<A_style, A_lead>(__VA_ARGS__); }         \
  default:                                                 \
    throw RedExceptExec("unsupported format");             \
  }


// generate template match/check/search functions with different prototypes
#define ZEZAX_RED_FUNC_DEFS(A_ret, A_func, ...)                          \
  template <Style style, bool doLeader>                                  \
  A_ret A_func(const Executable &exec, const void *ptr, size_t len) {    \
    RangeIter it(ptr, len);                                              \
    ZEZAX_RED_FMT_SWITCH(A_func ## Core, style, doLeader, __VA_ARGS__)   \
  }                                                                      \
  template <Style style, bool doLeader>                                  \
  A_ret A_func(const Executable &exec, const char *str) {                \
    NullTermIter it(str);                                                \
    ZEZAX_RED_FMT_SWITCH(A_func ## Core, style, doLeader, __VA_ARGS__)   \
  }                                                                      \
  template <Style style, bool doLeader>                                  \
  A_ret A_func(const Executable &exec, const std::string &s) {           \
    RangeIter it(s);                                                     \
    ZEZAX_RED_FMT_SWITCH(A_func ## Core, style, doLeader, __VA_ARGS__)   \
  }                                                                      \
  template <Style style, bool doLeader>                                  \
  A_ret A_func(const Executable &exec, std::string_view sv) {            \
    RangeIter it(sv);                                                    \
    ZEZAX_RED_FMT_SWITCH(A_func ## Core, style, doLeader, __VA_ARGS__)   \
  }

ZEZAX_RED_FUNC_DEFS(Result, check, exec, it, proxy)
ZEZAX_RED_FUNC_DEFS(Outcome, match, exec, it, proxy)
ZEZAX_RED_FUNC_DEFS(Result, scan, exec, it, proxy)
ZEZAX_RED_FUNC_DEFS(Outcome, search, exec, it, proxy)


// generate template replace functions with different prototypes
#define ZEZAX_RED_REPL_DEFS(A_ret, A_func, ...)                        \
  template <Style style, bool doLeader>                                \
  A_ret A_func(const Executable &exec, const void *ptr, size_t len,    \
               std::string_view repl, std::string &out, size_t max) {  \
    RangeIter it(ptr, len);                                            \
    ZEZAX_RED_FMT_SWITCH(A_func ## Core, style, doLeader, __VA_ARGS__) \
  }                                                                    \
  template <Style style, bool doLeader>                                \
  A_ret A_func(const Executable &exec, const char *str,                \
               std::string_view repl, std::string &out, size_t max) {  \
    NullTermIter it(str);                                              \
    ZEZAX_RED_FMT_SWITCH(A_func ## Core, style, doLeader, __VA_ARGS__) \
  }                                                                    \
  template <Style style, bool doLeader>                                \
  A_ret A_func(const Executable &exec, const std::string &s,           \
               std::string_view repl, std::string &out, size_t max) {  \
    RangeIter it(s);                                                   \
    ZEZAX_RED_FMT_SWITCH(A_func ## Core, style, doLeader, __VA_ARGS__) \
  }                                                                    \
  template <Style style, bool doLeader>                                \
  A_ret A_func(const Executable &exec, std::string_view sv,            \
               std::string_view repl, std::string &out, size_t max) {  \
    RangeIter it(sv);                                                  \
    ZEZAX_RED_FMT_SWITCH(A_func ## Core, style, doLeader, __VA_ARGS__) \
  }

ZEZAX_RED_REPL_DEFS(size_t, replace, exec, it, proxy, repl, out, max)

// don't #undef ZEZAX_RED_FMT_SWITCH
#undef ZEZAX_RED_FUNC_DEFS
#undef ZEZAX_RED_REPL_DEFS

///////////////////////////////////////////////////////////////////////////////
//
// ACTUAL CODE
//
///////////////////////////////////////////////////////////////////////////////

template <class InProxyT>
bool lookingAt(InProxyT    in, // by value
               const Byte *equivMap,
               const Byte *leader,
               size_t      leaderLen) {
  for (size_t ii = 0; ii < leaderLen; ++ii, ++in) {
    if (!in)
      return false;
    if (leader[ii] != equivMap[*in])
      return false;
  }
  return true;
}


template <class InProxyT>
bool compareThrough(InProxyT    &inOut, // by reference
                    const Byte *equivMap,
                    const Byte *leader,
                    size_t      leaderLen) {
  for (size_t ii = 0; ii < leaderLen; ++ii, ++inOut) {
    if (!inOut)
      return false;
    if (leader[ii] != equivMap[*inOut])
      return false;
  }
  return true;
}


template <Style style, bool doLeader, class InProxyT, class DfaProxyT>
Result checkCore(const Executable &exec, InProxyT in, DfaProxyT dfap) {
  const FileHeader *hdr = exec.getHeader();
  const char *__restrict__ base = exec.getBase();
  const Byte *__restrict__ equivMap = exec.getEquivMap();

  // if there's a required leader, fail if it's not there
  if (doLeader) {
    const Byte *__restrict__ leader = exec.getLeader();
    if (!compareThrough(in, equivMap, leader, exec.getLeaderLen()))
      return 0;
    dfap.init(base, hdr->leaderOff_);
  }
  else
    dfap.init(base, hdr->initialOff_);

  Result result = dfap.result();
  Result prevResult = 0;

  for (; in; ++in) {
    Byte byte = equivMap[*in];
    dfap.next(base, byte);
    result = dfap.result();
    if (UNLIKELY(result > 0)) {
      if (style == styInstant)
        return result;
      if (style == styFirst) {
        if (prevResult && (result != prevResult))
          return prevResult;
        prevResult = result;
      }
      if ((style == styTangent) || (style == styLast))
        prevResult = result;
    }
    else {
      if (((style == styFirst) || (style == styTangent)) && (prevResult > 0))
        return prevResult;
      if (dfap.pureDeadEnd())
        break;
    }
  }

  if (style == styLast)
    if ((result == 0) && (prevResult > 0))
      return prevResult;

  return result;
}


template <Style style, bool doLeader, class InProxyT, class DfaProxyT>
Outcome matchCore(const Executable &exec, InProxyT in, DfaProxyT dfap) {
  typedef typename decltype(dfap)::State State;

  Outcome rv;

  const FileHeader *hdr = exec.getHeader();
  const char *__restrict__ base = exec.getBase();
  const Byte *__restrict__ equivMap = exec.getEquivMap();

  // if there's a required leader, fail if it's not there
  if (doLeader) {
    const Byte *__restrict__ leader = exec.getLeader();
    size_t leaderLen = exec.getLeaderLen();
    if (!lookingAt(in, equivMap, leader, leaderLen)) {
      rv.result_ = 0;
      rv.start_ = 0;
      rv.end_ = 0;
      return rv;
    }
  }

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
      if (style == styFirst) {
        if (prevResult && (result != prevResult)) {
          result = prevResult;
          break;
        }
        prevResult = result;
      }
      matchEnd = idx + 1;
      if (style == styInstant)
        break;
      if ((style == styTangent) || (style == styLast))
        prevResult = result;
    }
    else {
      if ((style == styFirst) && (prevResult > 0)) {
        result = prevResult;
        break;
      }
      if ((style == styTangent) && (prevResult > 0))
        break;
      if (dfap.pureDeadEnd())
        break;
    }
  }

  if ((style == styTangent) || (style == styLast))
    if ((result == 0) && (prevResult > 0))
      result = prevResult;

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


template <Style style, bool doLeader, class InProxyT, class DfaProxyT>
Result scanCore(const Executable &exec, InProxyT in, DfaProxyT dfap) {
  const FileHeader *hdr = exec.getHeader();
  const char *__restrict__ base = exec.getBase();
  const Byte *__restrict__ equivMap = exec.getEquivMap();
  const Byte *__restrict__ leader = exec.getLeader();
  size_t leaderLen = exec.getLeaderLen();

  dfap.init(base, hdr->initialOff_);
  Result result = dfap.result();
  DfaProxyT leaderp;
  leaderp.init(base, hdr->leaderOff_);

  for (; in; ++in) {
    DfaProxyT dproxy;
    if (doLeader) {
      if (!compareThrough(in, equivMap, leader, leaderLen))
        continue;
      dproxy = leaderp;
      result = dproxy.result();
    }
    else
      dproxy = dfap;

    Result prevResult = 0;
    for (InProxyT inner(in); inner; ++inner) {
      Byte byte = equivMap[*inner];
      dproxy.next(base, byte);
      result = dproxy.result();
      if (UNLIKELY(result > 0)) {
        if (style == styInstant)
          return result;
        if (style == styFirst) {
          if (prevResult && (result != prevResult))
            return prevResult;
          prevResult = result;
        }
        if ((style == styTangent) || (style == styLast))
          prevResult = result;
      }
      else {
        if (((style == styFirst) || (style == styTangent)) && (prevResult > 0))
          return prevResult;
        if (dproxy.pureDeadEnd())
          break;
      }
    }

    if (style == styLast)
      if ((result == 0) && (prevResult > 0))
        return prevResult;
    if (result > 0)
      return result;
  }

  return result;
}


template <Style style, bool doLeader, class InProxyT, class DfaProxyT>
Outcome searchCore(const Executable &exec, InProxyT in, DfaProxyT dfap) {
  typedef typename DfaProxyT::State State;

  Outcome rv;

  const FileHeader *hdr = exec.getHeader();
  const char *__restrict__ base = exec.getBase();
  const Byte *__restrict__ equivMap = exec.getEquivMap();
  const Byte *__restrict__ leader = exec.getLeader();
  size_t leaderLen = exec.getLeaderLen();

  dfap.init(base, hdr->initialOff_);
  const State *__restrict__ init = dfap.state();
  Result result = dfap.result();
  size_t idx = 0;
  size_t matchStart = 0;
  size_t matchEnd = 0;

  for (; in; ++in, ++idx) {
    if (doLeader && !lookingAt(in, equivMap, leader, leaderLen))
      continue;

    DfaProxyT dproxy = dfap;
    Result prevResult = 0;
    size_t innerIdx = idx;
    matchStart = idx;
    matchEnd = idx;
    for (InProxyT inner(in); inner; ++inner, ++innerIdx) {
      Byte byte = equivMap[*inner];

      if (UNLIKELY(dproxy.state() == init)) {
        const State *__restrict__ prevState = dproxy.state();
        dproxy.next(base, byte);
        if (dproxy.state() != prevState)
          matchStart = innerIdx;
      }
      else
        dproxy.next(base, byte);
      result = dproxy.result();
      if (UNLIKELY(result > 0)) {
        if (style == styFirst) {
          if (prevResult && (result != prevResult)) {
            result = prevResult;
            break;
          }
          prevResult = result;
        }
        matchEnd = innerIdx + 1;
        if (style == styInstant)
          break;
        if ((style == styTangent) || (style == styLast))
          prevResult = result;
      }
      else {
        if ((style == styFirst) && (prevResult > 0)) {
          result = prevResult;
          break;
        }
        if ((style == styTangent) && (prevResult > 0))
          break;
        if (dproxy.pureDeadEnd())
          break;
      }
    }

    if ((style == styTangent) || (style == styLast))
      if ((result == 0) && (prevResult > 0))
        result = prevResult;
    if (result > 0)
      break;
  }

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


template <Style style, bool doLeader, class InProxyT, class DfaProxyT>
size_t replaceCore(const Executable &exec,
                   InProxyT          in,
                   DfaProxyT         dfap,
                   std::string_view  repl,
                   std::string      &out,
                   size_t            max) {
  const FileHeader *hdr = exec.getHeader();
  const char *__restrict__ base = exec.getBase();
  const Byte *__restrict__ equivMap = exec.getEquivMap();
  const Byte *__restrict__ leader = exec.getLeader();
  size_t leaderLen = exec.getLeaderLen();

  dfap.init(base, hdr->initialOff_);
  out.clear();
  size_t cnt = 0;

  while (in) {
    if (cnt >= max) {
      for (; in; ++in)
        out += *in;
      break;
    }
    const Byte *__restrict__ found = nullptr;
    if (!doLeader || lookingAt(in, equivMap, leader, leaderLen)) {
      DfaProxyT dproxy = dfap;
      Result prevResult = 0;
      for (InProxyT inner(in); inner; ++inner) {
        Byte byte = equivMap[*inner];
        dproxy.next(base, byte);
        Result result = dproxy.result();
        if (UNLIKELY(result > 0)) {
          if (style == styFirst) {
            if (prevResult && (result != prevResult))
              break;
            prevResult = result;
          }
          found = inner.ptr();
          if (style == styInstant)
            break;
        }
        else {
          if (style == styFull)
            found = nullptr;
          if ((((style == styFirst) || (style == styTangent)) && found) ||
              dproxy.pureDeadEnd())
            break;
        }
      }
    }

    if (found) {
      out += repl;
      in = found + 1;
      ++cnt;
    }
    else {
      out += *in;
      ++in;
    }
  }

  return cnt;
}


// designed similar to RE2::Set::Match()
// FIXME: acts as styTangent
template <Style style, bool doLeader, class InProxyT, class DfaProxyT>
size_t matchAllCore(const Executable     &exec,
                    InProxyT              in,
                    DfaProxyT             dfap,
                    std::vector<Outcome> &out) {
  typedef typename decltype(dfap)::State State;

  const FileHeader *hdr = exec.getHeader();
  const char *__restrict__ base = exec.getBase();
  const Byte *__restrict__ equivMap = exec.getEquivMap();

  out.clear();
  if (doLeader) {
    const Byte *__restrict__ leader = exec.getLeader();
    size_t leaderLen = exec.getLeaderLen();
    if (!lookingAt(in, equivMap, leader, leaderLen))
      return 0;
  }

  dfap.init(base, hdr->initialOff_);
  const State *__restrict__ init = dfap.state();
  Result result = dfap.result();
  Result prevResult = 0;
  size_t idx = 0;
  size_t matchStart = 0;

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
      // FIXME: other styles
      if (result == prevResult)
        out.back().end_ = idx + 1;
      else {
        prevResult = result;
        out.emplace_back(Outcome{result, matchStart, idx + 1});
      }
    }
    else {
      if (dfap.pureDeadEnd())
        break;
      prevResult = 0;
    }
  }

  return out.size();
}


// this is slower but more flexible than the functions above
class StatefulMatcher {
public:
  StatefulMatcher(const Executable &exec); // exec must outlive this object

  Result advance(Byte input);
  Result result() const { return result_; }

private:
  template <class DfaProxyT> Result advanceCore(Byte input, DfaProxyT dfap) {
    Byte byte = equivMap_[input];
    dfap.restore(state_);
    dfap.next(base_, byte);
    state_ = dfap.state();
    result_ = dfap.result();
    return result_;
  }

  Format            fmt_;
  Result            result_;
  const char       *base_;
  const void       *state_;
  const Byte       *equivMap_;
};

} // namespace zezax::red
