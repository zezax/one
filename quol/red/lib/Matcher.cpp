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


Result Matcher::checkWhole(const void *ptr, size_t len) {
  RangeIter it(ptr, len);
  switch (fmt_) {
  case fmtOffset1:
    {
      DfaProxy<fmtOffset1> proxy;
      return checkWholeX(it, proxy);
    }
  case fmtOffset2:
    {
      DfaProxy<fmtOffset2> proxy;
      return checkWholeX(it, proxy);
    }
  case fmtOffset4:
    {
      DfaProxy<fmtOffset4> proxy;
      return checkWholeX(it, proxy);
    }
  default:
    throw RedExcept("unsupported format");
  }
}


Result Matcher::checkWhole(const char *str) {
  NullTermIter it(str);
  switch (fmt_) {
  case fmtOffset1:
    {
      DfaProxy<fmtOffset1> proxy;
      return checkWholeX(it, proxy);
    }
  case fmtOffset2:
    {
      DfaProxy<fmtOffset2> proxy;
      return checkWholeX(it, proxy);
    }
  case fmtOffset4:
    {
      DfaProxy<fmtOffset4> proxy;
      return checkWholeX(it, proxy);
    }
  default:
    throw RedExcept("unsupported format");
  }
}


Result Matcher::checkWhole(const string &s) {
  RangeIter it(s);
  switch (fmt_) {
  case fmtOffset1:
    {
      DfaProxy<fmtOffset1> proxy;
      return checkWholeX(it, proxy);
    }
  case fmtOffset2:
    {
      DfaProxy<fmtOffset2> proxy;
      return checkWholeX(it, proxy);
    }
  case fmtOffset4:
    {
      DfaProxy<fmtOffset4> proxy;
      return checkWholeX(it, proxy);
    }
  default:
    throw RedExcept("unsupported format");
  }
}


Result Matcher::checkWhole(const string_view sv) {
  RangeIter it(sv);
  switch (fmt_) {
  case fmtOffset1:
    {
      DfaProxy<fmtOffset1> proxy;
      return checkWholeX(it, proxy);
    }
  case fmtOffset2:
    {
      DfaProxy<fmtOffset2> proxy;
      return checkWholeX(it, proxy);
    }
  case fmtOffset4:
    {
      DfaProxy<fmtOffset4> proxy;
      return checkWholeX(it, proxy);
    }
  default:
    throw RedExcept("unsupported format");
  }
}


Result Matcher::matchLong(const void *ptr, size_t len) {
  RangeIter it(ptr, len);
  switch (fmt_) {
  case fmtOffset1:
    {
      DfaProxy<fmtOffset1> proxy;
      return matchLongX(it, proxy);
    }
  case fmtOffset2:
    {
      DfaProxy<fmtOffset2> proxy;
      return matchLongX(it, proxy);
    }
  case fmtOffset4:
    {
      DfaProxy<fmtOffset4> proxy;
      return matchLongX(it, proxy);
    }
  default:
    throw RedExcept("unsupported format");
  }
}


Result Matcher::matchLong(const char *str) {
  NullTermIter it(str);
  switch (fmt_) {
  case fmtOffset1:
    {
      DfaProxy<fmtOffset1> proxy;
      return matchLongX(it, proxy);
    }
  case fmtOffset2:
    {
      DfaProxy<fmtOffset2> proxy;
      return matchLongX(it, proxy);
    }
  case fmtOffset4:
    {
      DfaProxy<fmtOffset4> proxy;
      return matchLongX(it, proxy);
    }
  default:
    throw RedExcept("unsupported format");
  }
}


Result Matcher::matchLong(const string &s) {
  RangeIter it(s);
  switch (fmt_) {
  case fmtOffset1:
    {
      DfaProxy<fmtOffset1> proxy;
      return matchLongX(it, proxy);
    }
  case fmtOffset2:
    {
      DfaProxy<fmtOffset2> proxy;
      return matchLongX(it, proxy);
    }
  case fmtOffset4:
    {
      DfaProxy<fmtOffset4> proxy;
      return matchLongX(it, proxy);
    }
  default:
    throw RedExcept("unsupported format");
  }
}


Result Matcher::matchLong(const string_view sv) {
  RangeIter it(sv);
  switch (fmt_) {
  case fmtOffset1:
    {
      DfaProxy<fmtOffset1> proxy;
      return matchLongX(it, proxy);
    }
  case fmtOffset2:
    {
      DfaProxy<fmtOffset2> proxy;
      return matchLongX(it, proxy);
    }
  case fmtOffset4:
    {
      DfaProxy<fmtOffset4> proxy;
      return matchLongX(it, proxy);
    }
  default:
    throw RedExcept("unsupported format");
  }
}

} // namespace zezax::red
