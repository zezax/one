// optimized format for dfa

#include "Serializer.h"

#include <cstring>

#include "Except.h"
#include "Fnv.h"
#include "Util.h"
#include "Proxy.h"

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

  switch (fmt) {
  case fmtOffset1:
    DfaProxy<fmtOffset1>::checkCapacity(dfa_.numStates(), maxChar_, maxResult_);
    break;
  case fmtOffset2:
    DfaProxy<fmtOffset2>::checkCapacity(dfa_.numStates(), maxChar_, maxResult_);
    break;
  case fmtOffset4:
    DfaProxy<fmtOffset4>::checkCapacity(dfa_.numStates(), maxChar_, maxResult_);
    break;
  default:
    throw RedExcept("unsuitable DFA format requested");
  }

  return fmt;
}


Format Serializer::optimalFormat() {
  Format res;
  if (!DfaProxy<fmtOffset4>::resultFits(maxResult_))
    throw RedExceptLimit("max result too big for any format");
  else if (!DfaProxy<fmtOffset2>::resultFits(maxResult_))
    res = fmtOffset4;
  else if (!DfaProxy<fmtOffset1>::resultFits(maxResult_))
    res = fmtOffset2;
  else
    res = fmtOffset1;

  Format off;
  if (!DfaProxy<fmtOffset4>::offsetFits(maxChar_, dfa_.numStates()))
    throw RedExceptLimit("num states too many for any format");
  else if (!DfaProxy<fmtOffset2>::offsetFits(maxChar_, dfa_.numStates()))
    off = fmtOffset4;
  else if (!DfaProxy<fmtOffset1>::offsetFits(maxChar_, dfa_.numStates()))
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
      DfaProxy<fmtOffset1> proxy;
      typename decltype(proxy)::State rec;
      rec.resultAndDeadEnd_ = proxy.resultAndDeadEnd(ds.result_, ds.deadEnd_);
      append(buf, &rec, sizeof(rec));
      for (CharIdx ch = 0; ch <= maxChar_; ++ch) {
        StateId id = ds.trans_[ch];
        size_t off = offsets_[id];
        proxy.appendOff(buf, off);
      }
    }
    break;

  case fmtOffset2:
    {
      DfaProxy<fmtOffset2> proxy;
      typename decltype(proxy)::State rec;
      rec.resultAndDeadEnd_ = proxy.resultAndDeadEnd(ds.result_, ds.deadEnd_);
      append(buf, &rec, sizeof(rec));
      for (CharIdx ch = 0; ch <= maxChar_; ++ch) {
        StateId id = ds.trans_[ch];
        size_t off = offsets_[id];
        proxy.appendOff(buf, off);
      }
    }
    break;

  case fmtOffset4:
    {
      DfaProxy<fmtOffset4> proxy;
      typename decltype(proxy)::State rec;
      rec.resultAndDeadEnd_ = proxy.resultAndDeadEnd(ds.result_, ds.deadEnd_);
      append(buf, &rec, sizeof(rec));
      for (CharIdx ch = 0; ch <= maxChar_; ++ch) {
        StateId id = ds.trans_[ch];
        size_t off = offsets_[id];
        proxy.appendOff(buf, off);
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
    return DfaProxy<fmtOffset1>::stateSize(maxChar_);
  case fmtOffset2:
    return DfaProxy<fmtOffset2>::stateSize(maxChar_);
  case fmtOffset4:
    return DfaProxy<fmtOffset4>::stateSize(maxChar_);
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
