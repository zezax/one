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
  prepareToSerialize();
  fmt = validatedFormat(fmt);
  return serializeToString(fmt);
}


void Serializer::serializeToFile(Format fmt, const char *path) {
  prepareToSerialize();
  fmt = validatedFormat(fmt);
  string buf = serializeToString(fmt);
  writeStringToFile(buf, path);
}


///////////////////////////////////////////////////////////////////////////////

void Serializer::prepareToSerialize() {
  if (dfa_.numStates() > 0xffffffff)
    throw RedExceptLimit("too many states for serialization format");
  findMaxChar();
  if (maxChar_ >= gAlphabetSize)
    throw RedExceptLimit("maxChar out of range");
  maxResult_ = dfa_.findMaxResult();
}


Format Serializer::validatedFormat(Format fmt) {
  if (fmt == fmtOffsetAuto)
    fmt = optimalFormat();

  // !!! assume result is same size as transition entries
  size_t tot = dfa_.numStates() * (maxChar_ + 2);

  switch (fmt) {
  case fmtOffset1:
    if ((maxResult_ > 0x7f) || (tot > 0xff))
      throw RedExceptLimit("DFA too big for 1-byte format");
    break;
  case fmtOffset2:
    if ((maxResult_ > 0x7fff) || (tot > 0xffff))
      throw RedExceptLimit("DFA too big for 2-byte format");
    break;
  case fmtOffset4:
    if ((maxResult_ > 0x7fffffff) || (tot > 0xffffffff))
      throw RedExceptLimit("DFA too big for 4-byte format");
    break;
  default:
    throw RedExcept("unsuitable DFA format requested");
  }

  return fmt;
}


Format Serializer::optimalFormat() {
  Format res;
  if (maxResult_ > 0x7fffffff)
    throw RedExceptLimit("max result too big for any format");
  else if (maxResult_ > 0x7fff)
    res = fmtOffset4;
  else if (maxResult_ > 0x7f)
    res = fmtOffset2;
  else
    res = fmtOffset1;

  size_t tot = dfa_.numStates() * (maxChar_ + 2);
  Format off;
  if (tot > 0xffffffff)
    throw RedExceptLimit("num states too many for any format");
  else if (tot > 0xffff)
    off = fmtOffset4;
  else if (tot > 0xff)
    off = fmtOffset2;
  else
    off = fmtOffset1;

  return std::max(res, off);
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
  size_t initOff = measureState(fmt, dfa_[gDfaInitialId]);
  if (initOff > 0xffffffff)
    throw RedExcept("initial offset too large");

  memset(&hdr, 0, sizeof(hdr));
  memcpy(hdr.magic_, "REDA", 4);
  hdr.majVer_ = 1;
  hdr.minVer_ = 0;
  hdr.format_ = fmt;
  hdr.maxChar_ = static_cast<uint8_t>(maxChar_);
  hdr.stateCnt_ = static_cast<uint32_t>(dfa_.numStates());
  hdr.initialOff_ = static_cast<uint32_t>(initOff);
  for (size_t ii = 0; ii < gAlphabetSize; ++ii)
    hdr.equivMap_[ii] = static_cast<uint8_t>(dfa_.getEquivMap()[ii]);
}


void Serializer::appendState(Format fmt, string &buf, const DfaState &ds) {
  switch (fmt) {

  case fmtOffset1:
    {
      StateOffset1 rec;
      rec.resultAndDeadEnd_ = (ds.result_ & 0x7f) | (ds.deadEnd_ << 7);
      append(buf, &rec, sizeof(rec));
      for (CharIdx ch = 0; ch <= maxChar_; ++ch) {
        StateId id = ds.trans_[ch];
        size_t off = offsets_[id];
        if (off > 0xff)
          throw RedExcept("overflow in 1-byte appendState");
        uint8_t off8 = static_cast<uint8_t>(off);
        append(buf, &off8, sizeof(off8));
      }
    }
    break;

  case fmtOffset2:
    {
      StateOffset2 rec;
      rec.resultAndDeadEnd_ = (ds.result_ & 0x7fff) | (ds.deadEnd_ << 15);
      append(buf, &rec, sizeof(rec));
      for (CharIdx ch = 0; ch <= maxChar_; ++ch) {
        StateId id = ds.trans_[ch];
        size_t off = offsets_[id];
        off >>= 1;
        if (off > 0xffff)
          throw RedExcept("overflow in 2-byte appendState");
        uint16_t off16 = static_cast<uint16_t>(off);
        append(buf, &off16, sizeof(off16));
      }
    }
    break;

  case fmtOffset4:
    {
      StateOffset4 rec;
      rec.resultAndDeadEnd_ = (ds.result_ & 0x7fffffff) | (ds.deadEnd_ << 31);
      append(buf, &rec, sizeof(rec));
      for (CharIdx ch = 0; ch <= maxChar_; ++ch) {
        StateId id = ds.trans_[ch];
        size_t off = offsets_[id];
        off >>= 2;
        if (off > 0xffffffff)
          throw RedExcept("overflow in 4-byte appendState");
        uint32_t off32 = static_cast<uint32_t>(off);
        append(buf, &off32, sizeof(off32));
      }
    }
    break;

  default:
    throw RedExcept("bad format in appendState");
  }
}


void Serializer::tabulateOffsets(Format fmt) {
  offsets_.clear();
  size_t off = 0;
  for (const DfaState &ds : dfa_.getStates()) {
    offsets_.push_back(off);
    off += measureState(fmt, ds);
  }
  offsets_.push_back(off); // just in case;
}


size_t Serializer::measureState(Format fmt, const DfaState &ds) {
  (void) ds;
  switch (fmt) {
  case fmtOffset1:
    return sizeof(StateOffset1) + (sizeof(uint8_t) * (maxChar_ + 1));
  case fmtOffset2:
    return sizeof(StateOffset2) + (sizeof(uint16_t) * (maxChar_ + 1));
  case fmtOffset4:
    return sizeof(StateOffset4) + (sizeof(uint32_t) * (maxChar_ + 1));
  default:
    throw RedExcept("bad format in measureState");
  }
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

  switch (hdr->format_) {
  case fmtOffset1:
  case fmtOffset2:
  case fmtOffset4:
    break;
  default:
    return "Serialized DFA: unsupported format";
  }

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
