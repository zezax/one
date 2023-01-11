// optimized format for dfa

#include "Serializer.h"

#include <cstring>

#include "Except.h"
#include "Fnv.h"
#include "Util.h"

namespace zezax::red {

using std::numeric_limits;
using std::string;

namespace {

void append(string &s, const void *p, size_t n) {
  s.append(static_cast<const char *>(p), n);
}

} // anonymous

Serializer::Serializer(const DfaObj &dfa)
: dfa_(dfa) {}


string Serializer::serialize(Format fmt) {
  ensureCanSerialize(fmt);
  return serializeToString(fmt);
}


void Serializer::serializeToFile(Format fmt, const char *path) {
  ensureCanSerialize(fmt);
  string buf = serializeToString(fmt);
  writeStringToFile(buf, path);
}


///////////////////////////////////////////////////////////////////////////////

void Serializer::ensureCanSerialize(Format fmt) {
  if (fmt != fmtOffset4)
    throw RedExcept("unsupported format for serialize");
  size_t stateCnt = dfa_.getStates().size();
  if (stateCnt > 0xffffffff)
    throw RedExcept("too many states for serialization format");
  findMaxChar();
  if (maxChar_ >= gAlphabetSize)
    throw RedExcept("maxChar out of range");
}


string Serializer::serializeToString(Format fmt) {
  tabulateOffsets(fmt);

  FileHeader hdr;
  populateHeader(hdr, fmt);

  string buf;
  append(buf, &hdr, sizeof(hdr));

  for (const DfaState &ds : dfa_.getStates())
    appendState(fmt, buf, ds);

  FileHeader *hdrp = reinterpret_cast<FileHeader *>(buf.data());
  hdrp->checksum_ = calcChecksum(buf.data(), buf.size());

  buf.shrink_to_fit();
  return buf;
}


void Serializer::populateHeader(FileHeader &hdr, Format fmt) {
  memset(&hdr, 0, sizeof(hdr));
  memcpy(hdr.magic_, "REDA", 4);
  hdr.majVer_ = 1;
  hdr.minVer_ = 0;
  hdr.format_ = fmt;
  hdr.maxChar_ = static_cast<uint8_t>(maxChar_);
  hdr.stateCnt_ = static_cast<uint32_t>(dfa_.getStates().size());
  hdr.initialOff_ = measureState(fmt, dfa_[gDfaInitialId]);
  for (size_t ii = 0; ii < gAlphabetSize; ++ii)
    hdr.equivMap_[ii] = static_cast<uint8_t>(dfa_.getEquivMap()[ii]);
}


void Serializer::appendState(Format fmt, string &buf, const DfaState &ds) {
  (void) fmt;
  StateOffset4 rec;
  rec.resultAndDeadEnd_ = (ds.result_ & 0x7fffffff) | (ds.deadEnd_ << 31);
  append(buf, &rec, sizeof(rec));
  for (CharIdx ch = 0; ch <= maxChar_; ++ch) {
    StateId id = ds.trans_[ch];
    uint32_t off = offsets_[id];
    off >>= 2; // !!! offsets are divided by four in fmtOffset4
    append(buf, &off, sizeof(off));
  }
}


void Serializer::tabulateOffsets(Format fmt) {
  offsets_.clear();
  uint32_t off = 0;
  for (const DfaState &ds : dfa_.getStates()) {
    offsets_.push_back(off);
    off += measureState(fmt, ds);
  }
  offsets_.push_back(off); // just in case;
}


uint32_t Serializer::measureState(Format fmt, const DfaState &ds) {
  (void) ds;
  if (fmt != fmtOffset4)
    throw RedExcept("can only measure fmtOffset4");
  return static_cast<uint32_t>(sizeof(StateOffset4) +
                               (sizeof(uint32_t) * (maxChar_ + 1)));
}


void Serializer::findMaxChar() {
  CharIdx max = 0;
  for (CharIdx ch : dfa_.getEquivMap())
    if (ch > max)
      max = ch;
  maxChar_ = max;
}

///////////////////////////////////////////////////////////////////////////////

string loadFromFile(const char *path) {
  string str = readFileToString(path);
  if (str.empty())
    throw RedExcept("Serialized DFA file is empty");

  const char *msg = checkHeader(str.data(), str.size());
  if (msg)
    throw RedExcept(msg);

  return str;
}


const char *checkHeader(const void *ptr, size_t len) {
  if (len < sizeof(FileHeader))
    return "Serialized DFA: header too short";
  const char *buf = reinterpret_cast<const char *>(ptr);

  const FileHeader *hdr = reinterpret_cast<const FileHeader *>(buf);
  if ((hdr->magic_[0] != 'R') || (hdr->magic_[1] != 'E') ||
      (hdr->magic_[2] != 'D') || (hdr->magic_[3] != 'A'))
    return "Serialized DFA: bad magic number";
  if ((hdr->majVer_ != 1) || (hdr->minVer_ != 0))
    return "Serialized DFA: unrecognized version";

  if (hdr->checksum_ != calcChecksum(ptr, len))
    return "Serialized DFA: checksum mismatch";

  if (hdr->format_ != fmtOffset4)
    return "Serialized DFA: unsupported format";

  return nullptr;
}


uint32_t calcChecksum(const void *ptr, size_t len) {
  const char *buf = reinterpret_cast<const char *>(ptr);
  const char *end = buf + len;
  const FileHeader *hdr = reinterpret_cast<const FileHeader *>(buf);
  const char *start = reinterpret_cast<const char *>(&hdr->format_);
  return fnv1a<uint32_t>(start, end - start);
}

} // namespace zezax::red
