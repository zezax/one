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
      const StateOffset1 *state =
        reinterpret_cast<const StateOffset1 *>(base + hdr->initialOff_);
      result_ = state->resultAndDeadEnd_ & 0x7f;
    }
    break;
  case fmtOffset2:
    {
      const StateOffset2 *state =
        reinterpret_cast<const StateOffset2 *>(base + hdr->initialOff_);
      result_ = state->resultAndDeadEnd_ & 0x7fff;
    }
    break;
  case fmtOffset4:
    {
      const StateOffset4 *state =
        reinterpret_cast<const StateOffset4 *>(base + hdr->initialOff_);
      result_ = state->resultAndDeadEnd_ & 0x7fffffff;
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
    return checkWhole1(it);
  case fmtOffset2:
    return checkWhole2(it);
  case fmtOffset4:
    return checkWhole4(it);
  default:
    throw RedExcept("unsupported format");
  }
}


Result Matcher::checkWhole(const char *str) {
  NullTermIter it(str);
  switch (fmt_) {
  case fmtOffset1:
    return checkWhole1(it);
  case fmtOffset2:
    return checkWhole2(it);
  case fmtOffset4:
    return checkWhole4(it);
  default:
    throw RedExcept("unsupported format");
  }
}


Result Matcher::checkWhole(const string &s) {
  RangeIter it(s);
  switch (fmt_) {
  case fmtOffset1:
    return checkWhole1(it);
  case fmtOffset2:
    return checkWhole2(it);
  case fmtOffset4:
    return checkWhole4(it);
  default:
    throw RedExcept("unsupported format");
  }
}


Result Matcher::checkWhole(const string_view sv) {
  RangeIter it(sv);
  switch (fmt_) {
  case fmtOffset1:
    return checkWhole1(it);
  case fmtOffset2:
    return checkWhole2(it);
  case fmtOffset4:
    return checkWhole4(it);
  default:
    throw RedExcept("unsupported format");
  }
}


Result Matcher::matchLong(const void *ptr, size_t len) {
  RangeIter it(ptr, len);
  switch (fmt_) {
  case fmtOffset1:
    return matchLong1(it);
  case fmtOffset2:
    return matchLong2(it);
  case fmtOffset4:
    return matchLong4(it);
  default:
    throw RedExcept("unsupported format");
  }
}


Result Matcher::matchLong(const char *str) {
  NullTermIter it(str);
  switch (fmt_) {
  case fmtOffset1:
    return matchLong1(it);
  case fmtOffset2:
    return matchLong2(it);
  case fmtOffset4:
    return matchLong4(it);
  default:
    throw RedExcept("unsupported format");
  }
}


Result Matcher::matchLong(const string &s) {
  RangeIter it(s);
  switch (fmt_) {
  case fmtOffset1:
    return matchLong1(it);
  case fmtOffset2:
    return matchLong2(it);
  case fmtOffset4:
    return matchLong4(it);
  default:
    throw RedExcept("unsupported format");
  }
}


Result Matcher::matchLong(const string_view sv) {
  RangeIter it(sv);
  switch (fmt_) {
  case fmtOffset1:
    return matchLong1(it);
  case fmtOffset2:
    return matchLong2(it);
  case fmtOffset4:
    return matchLong4(it);
  default:
    throw RedExcept("unsupported format");
  }
}

} // namespace zezax::red
