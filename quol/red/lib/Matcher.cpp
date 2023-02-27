// fast matcher using serialized dfa representation - implementation

#include "Matcher.h"

#include "Except.h"

namespace zezax::red {

using std::string;
using std::string_view;


Matcher::Matcher(std::shared_ptr<const Executable> exec)
  : exec_(exec.get()), shared_(std::move(exec)) {
  reset();
}


Matcher::Matcher(const Executable *exec)
  : exec_(exec) {
  reset();
}


void Matcher::reset() {
  matchStart_ = 0;
  matchEnd_   = 0;

  const FileHeader *hdr = exec_->getHeader();
  const char *base = exec_->getBase();
  fmt_ = static_cast<Format>(hdr->format_);
  switch (fmt_) {
  case fmtDirect1:
    {
      DfaProxy<fmtDirect1> proxy;
      proxy.init(base, hdr->initialOff_);
      result_ = proxy.result();
    }
    break;
  case fmtDirect2:
    {
      DfaProxy<fmtDirect2> proxy;
      proxy.init(base, hdr->initialOff_);
      result_ = proxy.result();
    }
    break;
  case fmtDirect4:
    {
      DfaProxy<fmtDirect4> proxy;
      proxy.init(base, hdr->initialOff_);
      result_ = proxy.result();
    }
    break;
  default:
    throw RedExceptExec("unsupported format");
  }
}


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
#define SUITE(A_name)                                                 \
  Result Matcher::A_name(const void *ptr, size_t len, Style style) {  \
    STYLE_SWITCH(A_name, M, CASE, ptr, len)                           \
  }                                                                   \
  Result Matcher::A_name(const char *str, Style style) {              \
    STYLE_SWITCH(A_name, M, CASE, str)                                \
  }                                                                   \
  Result Matcher::A_name(const string &s, Style style) {              \
    STYLE_SWITCH(A_name, M, CASE, s)                                  \
  }                                                                   \
  Result Matcher::A_name(string_view sv, Style style) {               \
    STYLE_SWITCH(A_name, M, CASE, sv)                                 \
  }


SUITE(check)
SUITE(match)


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
