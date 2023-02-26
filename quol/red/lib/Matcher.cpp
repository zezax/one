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
#define MCASE(A_name, A_len, ...)      \
  case A_len:                          \
    return A_name<A_len>(__VA_ARGS__);


// for replace
#define RCASE(A_name, A_len, ...)                             \
  case A_len:                                                 \
    ZEZAX_RED_FMT_SWITCH(A_name ## Core, A_len, __VA_ARGS__);


// runtime dispatch based on match-length; concatenate pfx & suf to expand
#define LEN_SWITCH(A_name, A_pfx, A_suf, ...)            \
  switch(mlen) {                                         \
    A_pfx ## A_suf(A_name, lenShortest,   __VA_ARGS__)   \
    A_pfx ## A_suf(A_name, lenContiguous, __VA_ARGS__)   \
    A_pfx ## A_suf(A_name, lenLast,       __VA_ARGS__)   \
    A_pfx ## A_suf(A_name, lenFull,       __VA_ARGS__)   \
  default:                                               \
    throw RedExceptExec("unsupported length");           \
  }


// generate match/check functions with different prototypes
#define SUITE(A_name)                                                 \
  Result Matcher::A_name(const void *ptr, size_t len, Length mlen) {  \
    LEN_SWITCH(A_name, M, CASE, ptr, len)                             \
  }                                                                   \
  Result Matcher::A_name(const char *str, Length mlen) {              \
    LEN_SWITCH(A_name, M, CASE, str)                                  \
  }                                                                   \
  Result Matcher::A_name(const string &s, Length mlen) {              \
    LEN_SWITCH(A_name, M, CASE, s)                                    \
  }                                                                   \
  Result Matcher::A_name(string_view sv, Length mlen) {               \
    LEN_SWITCH(A_name, M, CASE, sv)                                   \
  }


SUITE(check)
SUITE(match)


// generate replace functions with different prototypes
#define REPL(A_name, ...)                                                   \
  string Matcher::A_name(const void *ptr, size_t len,                       \
                         string_view repl, Length mlen) {                   \
    RangeIter it(ptr, len);                                                 \
    LEN_SWITCH(A_name, R, CASE, __VA_ARGS__)                                \
  }                                                                         \
  string Matcher::A_name(const char *str, string_view repl, Length mlen) {  \
    NullTermIter it(str);                                                   \
    LEN_SWITCH(A_name, R, CASE, __VA_ARGS__)                                \
  }                                                                         \
  string Matcher::A_name(const string &s, string_view repl, Length mlen) {  \
    RangeIter it(s);                                                        \
    LEN_SWITCH(A_name, R, CASE, __VA_ARGS__)                                \
  }                                                                         \
  string Matcher::A_name(string_view sv, string_view repl, Length mlen) {   \
    RangeIter it(sv);                                                       \
    LEN_SWITCH(A_name, R, CASE, __VA_ARGS__)                                \
  }


REPL(replace, it, proxy, repl)

} // namespace zezax::red
