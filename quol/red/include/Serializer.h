// optimized format for dfa

#pragma once

#include <string>

#include "Defs.h"
#include "Dfa.h"

namespace zezax::red {

enum Format : uint8_t {
  fmtTiny1   = 1,
  fmtTiny2   = 2,
  fmtOffset2 = 3,
  fmtOffset3 = 4,
  fmtOffset4 = 5, // only one for now
};

struct FileHeader {
  uint8_t  magic_[4]; // "REDA"
  uint16_t majVer_;
  uint16_t minVer_;
  uint32_t checksum_; // FNV-1a of all that follows
  uint8_t  format_;
  uint8_t  maxChar_;
  uint16_t pad0_;
  uint32_t stateCnt_;
  uint32_t initialOff_;
  uint8_t  equivMap_[256];
  uint8_t  bytes_[0]; // gcc-ism; offsets are from here
};


struct StateOffset4 {
  uint32_t resultAndDeadEnd_; // low 31 bits are result
  uint32_t offsets_[0]; // gcc-ism
};


class Serializer {
public:
  Serializer(const DfaObj &dfa);

  std::string serialize(Format fmt);
  void serializeToFile(Format fmt, const char *path);

private:
  void ensureCanSerialize(Format fmt);
  std::string serializeToString(Format fmt);
  void populateHeader(FileHeader &hdr, Format fmt);
  void appendState(Format fmt, std::string &buf, const DfaState &ds);
  void tabulateOffsets(Format fmt);
  uint32_t measureState(Format fmt, const DfaState &ds);
  void findMaxChar();

  const DfaObj         &dfa_;
  CharIdx               maxChar_;
  std::vector<uint32_t> offsets_;
};


std::string loadFromFile(const char *path);
const char *checkHeader(const void *ptr, size_t len); // returns msg if bad
uint32_t calcChecksum(const void *ptr, size_t len);

} // namespace zezax::red
