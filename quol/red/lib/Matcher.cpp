// fast matcher using serialized dfa representation - implementation

#include "Matcher.h"

#include "Except.h"

namespace zezax::red {

using std::string;
using std::string_view;
using std::vector;

// Lots of macro magic here follows.  This reduces repetitive code and gives
// fewer places for special-case bugs to hide.

// for match/check
#define MCASE(A_name, A_style, ...)      \
  case A_style:                          \
    return A_name<A_style>(__VA_ARGS__);


// for replace
#define RCASE(A_name, A_style, ...)                             \
  case A_style:                                                 \
    ZEZAX_RED_FMT_SWITCH(A_name ## Core, A_style, __VA_ARGS__);


// runtime dispatch based on match-style; concatenate pfx & suf to expand
#define STYLE_SWITCH(A_name, A_pfx, A_suf, ...)          \
  switch(style) {                                        \
    A_pfx ## A_suf(A_name, styFirst,      __VA_ARGS__)   \
    A_pfx ## A_suf(A_name, styContiguous, __VA_ARGS__)   \
    A_pfx ## A_suf(A_name, styLast,       __VA_ARGS__)   \
    A_pfx ## A_suf(A_name, styFull,       __VA_ARGS__)   \
  default:                                               \
    throw RedExceptExec("unsupported style");            \
  }


// generate match/check functions with different prototypes
#define SUITE(A_ret, A_name)                                            \
  A_ret A_name(const Executable &exec,                                  \
               const void *ptr, size_t len, Style style) {              \
    STYLE_SWITCH(A_name, M, CASE, exec, ptr, len)                       \
  }                                                                     \
  A_ret A_name(const Executable &exec, const char *str, Style style) {  \
    STYLE_SWITCH(A_name, M, CASE, exec, str)                            \
  }                                                                     \
  A_ret A_name(const Executable &exec, const string &s, Style style) {  \
    STYLE_SWITCH(A_name, M, CASE, exec, s)                              \
  }                                                                     \
  A_ret A_name(const Executable &exec, string_view sv, Style style) {   \
    STYLE_SWITCH(A_name, M, CASE, exec, sv)                             \
  }


SUITE(Result, check)
SUITE(Outcome, match)
SUITE(Outcome, search)


// stuff for matchAll()
#define MALL_FMT(A_name, ...)                  \
  switch (exec.getFormat()) {                  \
  case fmtDirect1: {                           \
    DfaProxy<fmtDirect1> proxy;                \
    return A_name(__VA_ARGS__);                \
  }                                            \
  case fmtDirect2: {                           \
    DfaProxy<fmtDirect2> proxy;                \
    return A_name(__VA_ARGS__);                \
  }                                            \
  case fmtDirect4: {                           \
    DfaProxy<fmtDirect4> proxy;                \
    return A_name(__VA_ARGS__);                \
  }                                            \
  default:                                     \
    throw RedExceptExec("unsupported format"); \
  }


bool matchAll(const Executable &exec,
              const void       *ptr,
              size_t            len,
              vector<Outcome>  *out) {
  RangeIter it(ptr, len);
  MALL_FMT(matchAllCore, exec, it, proxy, out)
}


bool matchAll(const Executable &exec,
              const char       *str,
              vector<Outcome>  *out) {
  NullTermIter it(str);
  MALL_FMT(matchAllCore, exec, it, proxy, out)
}


bool matchAll(const Executable  &exec,
              std::string       &s,
              vector<Outcome>   *out) {
  RangeIter it(s);
  MALL_FMT(matchAllCore, exec, it, proxy, out)
}


bool matchAll(const Executable &exec,
              string_view       sv,
              vector<Outcome>  *out) {
  RangeIter it(sv);
  MALL_FMT(matchAllCore, exec, it, proxy, out)
}


// generate replace functions with different prototypes
#define REPL(A_name, ...)                                               \
  string A_name(const Executable &exec, const void *ptr, size_t len,    \
                string_view repl, Style style) {                        \
    RangeIter it(ptr, len);                                             \
    STYLE_SWITCH(A_name, R, CASE, __VA_ARGS__)                          \
  }                                                                     \
  string A_name(const Executable &exec, const char *str,                \
                string_view repl, Style style) {                        \
    NullTermIter it(str);                                               \
    STYLE_SWITCH(A_name, R, CASE, __VA_ARGS__)                          \
  }                                                                     \
  string A_name(const Executable &exec, const string &s,                \
                string_view repl, Style style) {                        \
    RangeIter it(s);                                                    \
    STYLE_SWITCH(A_name, R, CASE, __VA_ARGS__)                          \
  }                                                                     \
  string A_name(const Executable &exec, string_view sv,                 \
                string_view repl, Style style) {                        \
    RangeIter it(sv);                                                   \
    STYLE_SWITCH(A_name, R, CASE, __VA_ARGS__)                          \
  }


REPL(replace, true, exec, it, proxy, repl)

} // namespace zezax::red
