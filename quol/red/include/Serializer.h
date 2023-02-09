// optimized format for dfa

#pragma once

#include <string>

#include "Types.h"
#include "Dfa.h"

namespace zezax::red {

//FIXME: 3-byte, rename offset -> direct/location/address
enum Format : uint8_t {
  fmtOffsetAuto = 10,
  fmtOffset1    = 11,
  fmtOffset2    = 12,
  fmtOffset4    = 14,
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


struct StateOffset1 {
  uint8_t resultAndDeadEnd_; // low 7 bits are result
  uint8_t offsets_[0]; // gcc-ism
};


struct StateOffset2 {
  uint16_t resultAndDeadEnd_; // low 15 bits are result
  uint16_t offsets_[0]; // gcc-ism
};


struct StateOffset4 {
  uint32_t resultAndDeadEnd_; // low 31 bits are result
  uint32_t offsets_[0]; // gcc-ism
};


class Serializer {
public:
  explicit Serializer(const DfaObj &dfa, CompStats *stats = nullptr);

  std::string serialize(Format fmt);
  void serializeToFile(Format fmt, const char *path);

private:
  void prepareToSerialize();
  Format validatedFormat(Format fmt);
  Format optimalFormat();
  std::string serializeToString(Format fmt);
  void populateHeader(FileHeader &hdr, Format fmt);
  void appendState(Format fmt, std::string &buf, const DfaState &ds);
  void tabulateOffsets(Format fmt);
  size_t measureState(Format fmt, const DfaState &ds);
  void findMaxChar();

  const DfaObj       &dfa_;
  CharIdx             maxChar_;
  Result              maxResult_;
  std::vector<size_t> offsets_;
  CompStats          *stats_;
};


std::string loadFromFile(const char *path);
const char *checkHeader(const void *ptr, size_t len); // returns msg if bad
uint32_t calcChecksum(const void *ptr, size_t len);

} // namespace zezax::red
