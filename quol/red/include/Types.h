// type definition header for zezax::red

#pragma once

#include <cstddef>
#include <cstdint>

#include <chrono>

#include "BitSet.h"
#include "DefMap.h"
#include "SparseVec.h"

namespace zezax::red {

typedef uint8_t  Byte;
typedef uint32_t Flags;
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


// pass to both parse and compile stages if desired
struct CompStats {
  std::chrono::steady_clock::time_point preNfa_;
  std::chrono::steady_clock::time_point postNfa_;
  std::chrono::steady_clock::time_point preDfa_;
  std::chrono::steady_clock::time_point postDfa_;
  std::chrono::steady_clock::time_point preMinimize_;
  std::chrono::steady_clock::time_point postMinimize_;
  std::chrono::steady_clock::time_point preSerialize_;
  std::chrono::steady_clock::time_point postSerialize_;
  size_t                                numTokens_;
  size_t                                numPatterns_;
  size_t                                origNfaStates_;
  size_t                                usefulNfaStates_;
  size_t                                origDfaStates_;
  size_t                                minimizedDfaStates_;
  size_t                                serializedBytes_;
  size_t                                numDistinguishedSymbols_;
  size_t                                transitionTableRows_;
  size_t                                powersetMemUsed_;
};

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
