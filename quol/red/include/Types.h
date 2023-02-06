// type definition header for zezax::red

#pragma once

#include <cstddef>
#include <cstdint>

#include "BitSet.h"
#include "DefMap.h"
#include "CappedVec.h"

namespace zezax::red {

typedef uint8_t  Byte;
typedef int32_t  FlagsT;
typedef int32_t  Result;
typedef int32_t  NfaId;
typedef uint32_t CharIdx;
typedef int64_t  DfaId;


struct ResultTag {};
struct NfaTag {};
struct DfaTag {};

typedef BitSet<CharIdx, DefaultTag, uint32_t>       MultiChar;
typedef BitSet<CharIdx, DefaultTag, uint32_t>::Iter MultiCharIter;
typedef BitSet<Result, ResultTag>                   ResultSet;
typedef BitSet<Result, ResultTag>::Iter             ResultSetIter;
typedef BitSet<NfaId, NfaTag>                       NfaIdSet;
typedef BitSet<NfaId, NfaTag>::Iter                 NfaIdSetIter;
typedef BitSet<DfaId, DfaTag>                       DfaIdSet;
typedef BitSet<DfaId, DfaTag>::Iter                 DfaIdSetIter;

} // namespace zezax::red

// for use in std::unordered_map
template<> struct std::hash<zezax::red::MultiChar> {
  size_t operator()(const zezax::red::MultiChar &mc) const { return mc.hash(); }
};

template<> struct std::hash<zezax::red::NfaIdSet> {
  size_t operator()(const zezax::red::NfaIdSet &nis) const {
    return nis.hash();
  }
};

template<> struct std::hash<zezax::red::DfaIdSet> {
  size_t operator()(const zezax::red::DfaIdSet &sis) const {
    return sis.hash();
  }
};
