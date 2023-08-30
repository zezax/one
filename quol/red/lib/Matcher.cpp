/* Matcher.cpp - fast matcher using serialized dfa - implementation

   See general description in Matcher.h

   Herein are implemented the matching functions that operate on the
   serialized DFA representation encapsulated in Executable.

   There's a lot of macro and template gymnastics here in order to
   avoid repetition in the critical code.  Some of the templates come
   from Proxy.h, but most are here and in Matcher.h.

   A goal is to minimize run-time decision-making so that this code is
   suitable for performance-critical applications.  The compiler
   provides the leverage to generate all the specific-case functions
   that may be needed.
 */

#include "Matcher.h"

#include "Except.h"

namespace zezax::red {

using std::string;
using std::string_view;
using std::vector;

// Lots of macro magic here follows.  This reduces repetitive code and gives
// fewer places for special-case bugs to hide.

#define FCASE(A_name, A_style, A_lead, ...)      \
  case A_style:                                  \
    return A_name<A_style, A_lead>(__VA_ARGS__);


// runtime dispatch based on match-style
#define STYLE_SWITCH(A_name, ...)             \
  switch(style) {                             \
  FCASE(A_name, styInstant, __VA_ARGS__)      \
  FCASE(A_name, styFirst,   __VA_ARGS__)      \
  FCASE(A_name, styTangent, __VA_ARGS__)      \
  FCASE(A_name, styLast,    __VA_ARGS__)      \
  FCASE(A_name, styFull,    __VA_ARGS__)      \
  default:                                    \
    throw RedExceptExec("unsupported style"); \
  }


// generate match/check/search functions with different prototypes
#define SUITE(A_ret, A_name)                                            \
  A_ret A_name(const Executable &exec,                                  \
               const void *ptr, size_t len, Style style) {              \
    STYLE_SWITCH(A_name, true, exec, ptr, len)                          \
  }                                                                     \
  A_ret A_name(const Executable &exec, const char *str, Style style) {  \
    STYLE_SWITCH(A_name, true, exec, str)                               \
  }                                                                     \
  A_ret A_name(const Executable &exec, const string &s, Style style) {  \
    STYLE_SWITCH(A_name, true, exec, s)                                 \
  }                                                                     \
  A_ret A_name(const Executable &exec, string_view sv, Style style) {   \
    STYLE_SWITCH(A_name, true, exec, sv)                                \
  }


SUITE(Result, check)
SUITE(Outcome, match)
SUITE(Result, scan)
SUITE(Outcome, search)


// generate replace functions with different prototypes
#define REPL(A_ret, A_name, ...)                                          \
  A_ret A_name(const Executable &exec, const void *ptr, size_t len,       \
                string_view repl, string &out, size_t max, Style style) { \
    STYLE_SWITCH(A_name, true, exec, ptr, len, __VA_ARGS__)               \
  }                                                                       \
  A_ret A_name(const Executable &exec, const char *str,                   \
                string_view repl, string &out, size_t max, Style style) { \
    STYLE_SWITCH(A_name, true, exec, str, __VA_ARGS__)                    \
  }                                                                       \
  A_ret A_name(const Executable &exec, const string &s,                   \
                string_view repl, string &out, size_t max, Style style) { \
    STYLE_SWITCH(A_name, true, exec, s, __VA_ARGS__)                      \
  }                                                                       \
  A_ret A_name(const Executable &exec, string_view sv,                    \
                string_view repl, string &out, size_t max, Style style) { \
    STYLE_SWITCH(A_name, true, exec, sv, __VA_ARGS__)                     \
  }


REPL(size_t, replace, repl, out, max)


// stuff for matchAll() - just string_view for now

size_t matchAll(const Executable &exec,
                string_view       sv,
                vector<Outcome>  &out) {
  RangeIter it(sv);
  ZEZAX_RED_FMT_SWITCH(matchAllCore, styTangent, true, exec, it, proxy, out)
}

///////////////////////////////////////////////////////////////////////////////

StatefulMatcher::StatefulMatcher(const Executable &exec)
  : fmt_(exec.getFormat()),
    result_(0),
    base_(exec.getBase()),
    state_(nullptr),
    equivMap_(exec.getEquivMap()) {
  const FileHeader *hdr = exec.getHeader();
  switch (fmt_) {
  case fmtDirect1: {
    DfaProxy<fmtDirect1> proxy;
    proxy.init(base_, hdr->initialOff_);
    result_ = proxy.result();
    state_ = proxy.state();
    break;
  }
  case fmtDirect2: {
    DfaProxy<fmtDirect2> proxy;
    proxy.init(base_, hdr->initialOff_);
    result_ = proxy.result();
    state_ = proxy.state();
    break;
  }
  case fmtDirect4: {
    DfaProxy<fmtDirect4> proxy;
    proxy.init(base_, hdr->initialOff_);
    result_ = proxy.result();
    state_ = proxy.state();
    break;
  }
  default:
    throw RedExceptExec("unrecognized format");
  }
}


Result StatefulMatcher::advance(Byte input) {
  switch (fmt_) {
  case fmtDirect1: {
    DfaProxy<fmtDirect1> proxy;
    return advanceCore(input, proxy);
  }
  case fmtDirect2: {
    DfaProxy<fmtDirect2> proxy;
    return advanceCore(input, proxy);
  }
  case fmtDirect4: {
    DfaProxy<fmtDirect4> proxy;
    return advanceCore(input, proxy);
  }
  default:
    throw RedExceptExec("unrecognized format");
  }
}

} // namespace zezax::red
