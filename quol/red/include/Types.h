/* Types.h - type definition header for zezax::red

   This defines types that are generally applicable across the project.
   There is also some glue for using BitSets in unordered_map.
*/

#pragma once

#include <cstddef>
#include <cstdint>

#include <chrono>

#include "BitSet.h"
#include "DefaultMap.h"
#include "SparseVec.h"

namespace zezax::red {

typedef uint8_t  Byte;
typedef uint32_t Flags;
typedef int32_t  Result;
typedef int32_t  NfaId;
typedef uint32_t CharIdx;
typedef int64_t  DfaId;


// Tag Classes

// enforce non-interchangeability of various bit-sets...
struct ResultTag {};
struct NfaTag {};
struct DfaTag {};

// specify meaning of constructor arguments when not clear from context...

struct FlagsTag {}; // for passing a flags bitmask
constexpr FlagsTag gFlagsTag;

struct PathTag {}; // indicate that string is a path to a file/directory
constexpr PathTag gPathTag;

struct CopyTag {}; // indicate that a copy will be made and owned
constexpr CopyTag gCopyTag;

struct DeleteTag {}; // become owner of memory from new; will later delete
constexpr DeleteTag gDeleteTag;

struct FreeTag {}; // become owner of memory from malloc; clean up via free
constexpr FreeTag gFreeTag;

struct UnownedTag {}; // provide raw pointer with no ownership or cleanup
constexpr UnownedTag gUnownedTag;


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
  std::chrono::steady_clock::time_point postBasisChars_;
  std::chrono::steady_clock::time_point postMakeTable_;
  std::chrono::steady_clock::time_point postDfa_;
  std::chrono::steady_clock::time_point preMinimize_;
  std::chrono::steady_clock::time_point postEquivMap_;
  std::chrono::steady_clock::time_point postInvert_;
  std::chrono::steady_clock::time_point postPartition_;
  std::chrono::steady_clock::time_point postMakeList_;
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
