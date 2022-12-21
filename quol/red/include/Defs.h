// definitions header for zezax::red

#pragma once

#include <cstddef>
#include <cstdint>

namespace zezax::red {

typedef uint8_t  Byte;
typedef int32_t  FlagsT;
typedef int32_t  Result;
typedef uint32_t CharIdx;
typedef int64_t  StateId;

enum Flags : FlagsT {
  fIgnoreCase  = 0x01,
  fAnchorStart = 0x02,
  fAnchorEnd   = 0x04,
};

constexpr CharIdx gAlphabetSize = 256;
constexpr size_t gNoPos = ~0UL;

} // namespace zezax::red
