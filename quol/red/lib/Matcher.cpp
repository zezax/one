// fast matcher using serialized dfa representation - implementation

#include "Matcher.h"

#include "Except.h"
#include "Proxy.h"

namespace zezax::red {

using std::string;
using std::string_view;


Matcher::Matcher(std::shared_ptr<const Executable> exec)
  : exec_(std::move(exec)) {
  const FileHeader *hdr = exec_->getHeader();
  if (hdr->format_ != fmtOffset4)
    throw RedExcept("Unsupported format");
  reset();
}


void Matcher::reset() {
  matchStart_ = 0;
  matchEnd_ = 0;

  const FileHeader *hdr = exec_->getHeader();
  const char *base = exec_->getBase();
  if (hdr->format_ != fmtOffset4)
    throw RedExcept("Unsupported format");
  const StateOffset4 *state =
    reinterpret_cast<const StateOffset4 *>(base + hdr->initialOff_);
  result_ = state->resultAndDeadEnd_ & 0x7fffffff;
}


Result Matcher::checkShort(const void *ptr, size_t len) {
  RangeIter it(ptr, len);
  return checkShort4(it);
}


Result Matcher::checkShort(const char *str) {
  NullTermIter it(str);
  return checkShort4(it);
}


Result Matcher::checkShort(const string &s) {
  RangeIter it(s);
  return checkShort4(it);
}


Result Matcher::checkShort(const string_view sv) {
  RangeIter it(sv);
  return checkShort4(it);
}


Result Matcher::matchLong(const void *ptr, size_t len) {
  RangeIter it(ptr, len);
  return matchLong4(it);
}


Result Matcher::matchLong(const char *str) {
  NullTermIter it(str);
  return matchLong4(it);
}


Result Matcher::matchLong(const string &s) {
  RangeIter it(s);
  return matchLong4(it);
}


Result Matcher::matchLong(const string_view sv) {
  RangeIter it(sv);
  return matchLong4(it);
}

} // namespace zezax::red
