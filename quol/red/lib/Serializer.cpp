/* Serializer.cpp - creates optimized format for dfa - implementation

   See general description in Serializer.h

   This uses templates from Proxy.h in order to generate multiple
   formats from one code flow.
 */

#include "Serializer.h"

#include <cstring>

#include "Except.h"
#include "Fnv.h"
#include "Util.h"
#include "Proxy.h"

namespace zezax::red {

using std::string;
using std::vector;

namespace {

void append(string &s, const void *p, size_t n) {
  s.append(static_cast<const char *>(p), n);
}


void appendLeader(string &s, const string &leader) {
  s.append(leader);
  size_t size = leader.size();
  size_t pad = (size + 7) & ~7UL;
  for (size_t ii = size; ii < pad; ++ii) // round up to multiple of 8
    s.push_back(0);
}


template <Format fmt>
void appendStateImpl(std::string          &buf,
                     const DfaState       &ds,
                     CharIdx               maxChar,
                     const vector<size_t> &offsets) {
  DfaProxy<fmt> proxy;
  typename DfaProxy<fmt>::State rec;
  rec.resultAndDeadEnd_ = proxy.resultAndDeadEnd(ds.result_, ds.deadEnd_);
  append(buf, &rec, sizeof(rec));
  for (CharIdx ch = 0; ch <= maxChar; ++ch) {
    DfaId id = ds.transitions_[ch];
    size_t off = offsets[id];
    proxy.appendOffset(buf, off);
  }
}

} // anonymous

Serializer::Serializer(const DfaObj &dfa, CompStats *stats)
  : dfa_(dfa), stats_(stats) {}


string Serializer::serializeToString(Format fmt) {
  if (stats_)
    stats_->preSerialize_ = std::chrono::steady_clock::now();

  prepareToSerialize();
  fmt = validatedFormat(fmt);
  string buf = serialize(fmt);
  buf.shrink_to_fit();

  if (stats_) {
    stats_->serializedBytes_ = buf.size();
    stats_->postSerialize_   = std::chrono::steady_clock::now();
  }

  return buf;
}


void Serializer::serializeToFile(Format fmt, const char *path) {
  if (stats_)
    stats_->preSerialize_ = std::chrono::steady_clock::now();

  // we serialize to memory first, so we can populate the checksum
  prepareToSerialize();
  fmt = validatedFormat(fmt);
  string buf = serialize(fmt);
  writeStringToFile(buf, path);

  if (stats_) {
    stats_->serializedBytes_ = buf.size();
    stats_->postSerialize_   = std::chrono::steady_clock::now();
  }
}


///////////////////////////////////////////////////////////////////////////////

void Serializer::prepareToSerialize() {
  if (dfa_.numStates() > 0xffffffff)
    throw RedExceptLimit("too many states for serialization format");
  findMaxChar();
  if (maxChar_ >= gAlphabetSize)
    throw RedExceptLimit("maxChar out of range");
  maxResult_ = dfa_.findMaxResult();
  leader_ = dfa_.fixedPrefix(leaderNext_);
  if (leader_.size() > 255)
    throw RedExceptSerialize("leader too long");
}


Format Serializer::validatedFormat(Format fmt) {
  if (fmt == fmtDirectAuto)
    fmt = optimalFormat();

  switch (fmt) {
  case fmtDirect1:
    DfaProxy<fmtDirect1>::checkCapacity(dfa_.numStates(), maxChar_, maxResult_);
    break;
  case fmtDirect2:
    DfaProxy<fmtDirect2>::checkCapacity(dfa_.numStates(), maxChar_, maxResult_);
    break;
  case fmtDirect4:
    DfaProxy<fmtDirect4>::checkCapacity(dfa_.numStates(), maxChar_, maxResult_);
    break;
  default:
    throw RedExceptSerialize("unsuitable dfa format requested");
  }

  return fmt;
}


Format Serializer::optimalFormat() {
  Format res;
  if (!DfaProxy<fmtDirect4>::resultFits(maxResult_))
    throw RedExceptLimit("max result too big for any format");
  if (!DfaProxy<fmtDirect2>::resultFits(maxResult_))
    res = fmtDirect4;
  else if (!DfaProxy<fmtDirect1>::resultFits(maxResult_))
    res = fmtDirect2;
  else
    res = fmtDirect1;

  Format dir;
  if (!DfaProxy<fmtDirect4>::offsetFits(maxChar_, dfa_.numStates()))
    throw RedExceptLimit("num states too many for any format");
  if (!DfaProxy<fmtDirect2>::offsetFits(maxChar_, dfa_.numStates()))
    dir = fmtDirect4;
  else if (!DfaProxy<fmtDirect1>::offsetFits(maxChar_, dfa_.numStates()))
    dir = fmtDirect2;
  else
    dir = fmtDirect1;

  return std::max(res, dir);
}


string Serializer::serialize(Format fmt) {
  string buf;
  tabulateOffsets(fmt);

  {
    FileHeader hdr;
    populateHeader(hdr, fmt);
    append(buf, &hdr, sizeof(hdr));
  }

  appendLeader(buf, leader_);

  for (const DfaState &ds : dfa_.getStates())
    appendState(fmt, buf, ds);

  // patch up checksum
  FileHeader *hdrp = reinterpret_cast<FileHeader *>(buf.data());
  hdrp->checksum_ = calcChecksum(buf.data(), buf.size());

  return buf;
}


void Serializer::populateHeader(FileHeader &hdr, Format fmt) {
  size_t initOff = offsets_[gDfaInitialId];
  if (initOff > 0xffffffff)
    throw RedExceptSerialize("initial offset too large");
  size_t nextOff = offsets_[leaderNext_];
  if (nextOff > 0xffffffff)
    throw RedExceptSerialize("leader next offset too large");

  memset(&hdr, 0, sizeof(hdr));
  memcpy(hdr.magic_, "REDA", 4);
  hdr.majVer_     = 1;
  hdr.minVer_     = 0;
  hdr.format_     = fmt;
  hdr.maxChar_    = static_cast<uint8_t>(maxChar_);
  hdr.leaderLen_  = static_cast<uint8_t>(leader_.size());
  hdr.stateCnt_   = static_cast<uint32_t>(dfa_.numStates());
  hdr.initialOff_ = static_cast<uint32_t>(initOff);
  hdr.leaderOff_  = static_cast<uint32_t>(nextOff);
  for (size_t ii = 0; ii < gAlphabetSize; ++ii)
    hdr.equivMap_[ii] = static_cast<uint8_t>(dfa_.getEquivMap()[ii]);
}


void Serializer::appendState(Format fmt, string &buf, const DfaState &ds) {
  switch (fmt) {
  case fmtDirect1:
    appendStateImpl<fmtDirect1>(buf, ds, maxChar_, offsets_);
    break;
  case fmtDirect2:
    appendStateImpl<fmtDirect2>(buf, ds, maxChar_, offsets_);
    break;
  case fmtDirect4:
    appendStateImpl<fmtDirect4>(buf, ds, maxChar_, offsets_);
    break;
  default:
    throw RedExceptSerialize("bad format in appendState");
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


size_t Serializer::measureState(Format fmt, const DfaState &ds) const {
  (void) ds;
  switch (fmt) {
  case fmtDirect1:
    return DfaProxy<fmtDirect1>::stateSize(maxChar_);
  case fmtDirect2:
    return DfaProxy<fmtDirect2>::stateSize(maxChar_);
  case fmtDirect4:
    return DfaProxy<fmtDirect4>::stateSize(maxChar_);
  default:
    throw RedExceptSerialize("bad format in measureState");
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
    throw RedExceptApi("serialized dfa file is empty");

  const char *msg = checkHeader(str.data(), str.size());
  if (msg)
    throw RedExceptApi(msg);

  return str;
}


const char *checkHeader(const void *ptr, size_t len) {
  if (len < sizeof(FileHeader))
    return "Serialized DFA: header too short";

  const FileHeader *hdr = reinterpret_cast<const FileHeader *>(ptr);
  if ((hdr->magic_[0] != 'R') || (hdr->magic_[1] != 'E') ||
      (hdr->magic_[2] != 'D') || (hdr->magic_[3] != 'A'))
    return "Serialized DFA: bad magic number";
  if ((hdr->majVer_ != 1) || (hdr->minVer_ != 0))
    return "Serialized DFA: unrecognized version";

  uint32_t csum = calcChecksum(ptr, len);
  if (hdr->checksum_ != csum) {
    if (hdr->checksum_ == __builtin_bswap32(csum))
      return "serialized DFA: foreign endian-ness";
    return "serialized DFA: checksum mismatch";
  }

  switch (hdr->format_) {
  case fmtDirect1:
  case fmtDirect2:
  case fmtDirect4:
    break;
  default:
    return "Serialized DFA: unsupported format";
  }

  return nullptr;
}


uint32_t calcChecksum(const void *ptr, size_t len) {
  const FileHeader *hdr = reinterpret_cast<const FileHeader *>(ptr);
  const char *beg = reinterpret_cast<const char *>(&hdr->format_);
  const char *end = reinterpret_cast<const char *>(ptr) + len;
  return fnv1a<uint32_t>(beg, end - beg);
}

} // namespace zezax::red
