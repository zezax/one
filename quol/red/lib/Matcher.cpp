// fast matcher using serialized dfa representation - implementation

#include "Matcher.h"

#include "Except.h"

namespace zezax::red {

using std::string;
using std::string_view;


Matcher::Matcher(const Executable *exec)
  : exec_(exec),
    fmt_(static_cast<Format>(exec_->getHeader()->format_)) {}


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
#define SUITE(A_ret, A_name)                                          \
  A_ret Matcher::A_name(const void *ptr, size_t len, Style style) {   \
    STYLE_SWITCH(A_name, M, CASE, ptr, len)                           \
  }                                                                   \
  A_ret Matcher::A_name(const char *str, Style style) {               \
    STYLE_SWITCH(A_name, M, CASE, str)                                \
  }                                                                   \
  A_ret Matcher::A_name(const string &s, Style style) {               \
    STYLE_SWITCH(A_name, M, CASE, s)                                  \
  }                                                                   \
  A_ret Matcher::A_name(string_view sv, Style style) {                \
    STYLE_SWITCH(A_name, M, CASE, sv)                                 \
  }


SUITE(Result, check)
SUITE(Outcome, match)


// generate replace functions with different prototypes
#define REPL(A_name, ...)                                                   \
  string Matcher::A_name(const void *ptr, size_t len,                       \
                         string_view repl, Style style) {                   \
    RangeIter it(ptr, len);                                                 \
    STYLE_SWITCH(A_name, R, CASE, __VA_ARGS__)                              \
  }                                                                         \
  string Matcher::A_name(const char *str, string_view repl, Style style) {  \
    NullTermIter it(str);                                                   \
    STYLE_SWITCH(A_name, R, CASE, __VA_ARGS__)                              \
  }                                                                         \
  string Matcher::A_name(const string &s, string_view repl, Style style) {  \
    RangeIter it(s);                                                        \
    STYLE_SWITCH(A_name, R, CASE, __VA_ARGS__)                              \
  }                                                                         \
  string Matcher::A_name(string_view sv, string_view repl, Style style) {   \
    RangeIter it(sv);                                                       \
    STYLE_SWITCH(A_name, R, CASE, __VA_ARGS__)                              \
  }


REPL(replace, it, proxy, repl)

} // namespace zezax::red
