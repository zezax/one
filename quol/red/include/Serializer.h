// optimized format for dfa

#pragma once

#include <string>

#include "Types.h"
#include "Dfa.h"

namespace zezax::red {

enum Format : uint8_t {
  fmtInvalid    = 0,
  fmtDirect1    = 1,
  fmtDirect2    = 2,
  fmtDirect4    = 4,
  fmtDirectAuto = 255,
};

struct FileHeader {
  uint8_t  magic_[4]; // "REDA"
  uint16_t majVer_;
  uint16_t minVer_;
  uint32_t checksum_; // FNV-1a of all that follows
  uint8_t  format_;
  uint8_t  maxChar_;
  uint8_t  leaderLen_; // leader is a fixed prefix required by the dfa
  uint8_t  pad0_;
  uint32_t stateCnt_;
  uint32_t initialOff_;
  uint8_t  equivMap_[256];
  uint8_t  bytes_[0]; // gcc-ism; offsets start after leader
  // leader, if any, goes first, padded to 8-byte alignment
  // next, all the states in id order, as per format
};


struct StateDirect1 {
  uint8_t resultAndDeadEnd_; // low 7 bits are result
  uint8_t offsets_[0]; // gcc-ism
};


struct StateDirect2 {
  uint16_t resultAndDeadEnd_; // low 15 bits are result
  uint16_t offsets_[0]; // gcc-ism
};


struct StateDirect4 {
  uint32_t resultAndDeadEnd_; // low 31 bits are result
  uint32_t offsets_[0]; // gcc-ism
};


class Serializer {
public:
  explicit Serializer(const DfaObj &dfa, CompStats *stats = nullptr);

  std::string serializeToString(Format fmt);
  void serializeToFile(Format fmt, const char *path);

private:
  void prepareToSerialize();
  Format validatedFormat(Format fmt);
  Format optimalFormat();
  std::string serialize(Format fmt);
  void populateHeader(FileHeader &hdr, Format fmt);
  void appendState(Format fmt, std::string &buf, const DfaState &ds);
  void tabulateOffsets(Format fmt);
  size_t measureState(Format fmt, const DfaState &ds) const;
  void findMaxChar();

  const DfaObj        &dfa_;
  CharIdx              maxChar_;
  Result               maxResult_;
  std::string          leader_;
  std::vector<size_t>  offsets_;
  CompStats           *stats_;
};


std::string loadFromFile(const char *path);
const char *checkHeader(const void *ptr, size_t len); // returns msg if bad
uint32_t calcChecksum(const void *ptr, size_t len);

} // namespace zezax::red
