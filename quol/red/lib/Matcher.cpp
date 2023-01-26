// fast matcher using serialized dfa representation - implementation

#include "Matcher.h"

#include "Except.h"
#include "Proxy.h"

namespace zezax::red {

using std::string;
using std::string_view;


Matcher::Matcher(std::shared_ptr<const Executable> exec)
  : exec_(std::move(exec)) {
  reset();
}


void Matcher::reset() {
  matchStart_ = 0;
  matchEnd_ = 0;

  const Executable *exec = exec_.get();
  const FileHeader *hdr = exec->getHeader();
  const char *base = exec->getBase();
  fmt_ = static_cast<Format>(hdr->format_);
  switch (fmt_) {
  case fmtOffset1:
    {
      DfaProxy<fmtOffset1> proxy;
      const decltype(proxy)::State *state =
        proxy.stateAt(base, hdr->initialOff_);
      result_ = proxy.result(state);
    }
    break;
  case fmtOffset2:
    {
      DfaProxy<fmtOffset2> proxy;
      const decltype(proxy)::State *state =
        proxy.stateAt(base, hdr->initialOff_);
      result_ = proxy.result(state);
    }
    break;
  case fmtOffset4:
    {
      DfaProxy<fmtOffset4> proxy;
      const decltype(proxy)::State *state =
        proxy.stateAt(base, hdr->initialOff_);
      result_ = proxy.result(state);
    }
    break;
  default:
    throw RedExcept("Unsupported format");
  }
}


#define BODY(A_func, A_len)                     \
  switch (fmt_) {                               \
  case fmtOffset1: {                            \
    DfaProxy<fmtOffset1> proxy;                 \
    return A_func<A_len>(it, proxy); }          \
  case fmtOffset2: {                            \
    DfaProxy<fmtOffset2> proxy;                 \
    return A_func<A_len>(it, proxy); }          \
  case fmtOffset4: {                            \
    DfaProxy<fmtOffset4> proxy;                 \
    return A_func<A_len>(it, proxy); }          \
  default:                                      \
    throw RedExcept("unsupported format");      \
  }


#define PROTOS(A_pfx, A_suf, A_len)                                \
  Result Matcher::A_pfx ## A_suf(const void *ptr, size_t len) {    \
    RangeIter it(ptr, len);                                        \
    BODY(A_pfx ## Core, A_len)                                     \
  }                                                                \
  Result Matcher::A_pfx ## A_suf(const char *str) {                \
    NullTermIter it(str);                                          \
    BODY(A_pfx ## Core, A_len)                                     \
  }                                                                \
  Result Matcher::A_pfx ## A_suf(const string &s) {                \
    RangeIter it(s);                                               \
    BODY(A_pfx ## Core, A_len)                                     \
  }                                                                \
  Result Matcher::A_pfx ## A_suf(const string_view sv) {           \
    RangeIter it(sv);                                              \
    BODY(A_pfx ## Core, A_len)                                     \
  }


#define SUITE(A_prefix)                         \
  PROTOS(A_prefix, Short, lenShortest)          \
  PROTOS(A_prefix, Contig, lenContiguous)       \
  PROTOS(A_prefix, Last, lenLast)               \
  PROTOS(A_prefix, Whole, lenWhole)


SUITE(check)
SUITE(match)


#define REPL(A_func, A_len)                     \
  switch (fmt_) {                               \
  case fmtOffset1: {                            \
    DfaProxy<fmtOffset1> proxy;                 \
    return A_func<A_len>(it, proxy, repl); }    \
  case fmtOffset2: {                            \
    DfaProxy<fmtOffset2> proxy;                 \
    return A_func<A_len>(it, proxy, repl); }    \
  case fmtOffset4: {                            \
    DfaProxy<fmtOffset4> proxy;                 \
    return A_func<A_len>(it, proxy, repl); }    \
  default:                                      \
    throw RedExcept("unsupported format");      \
  }

string Matcher::replaceLast(const string &src, const string &repl) {
  RangeIter it(src);
  REPL(replaceCore, lenLast)
}

} // namespace zezax::red
