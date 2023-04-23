// fast matcher using serialized dfa representation - implementation

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

} // namespace zezax::red
