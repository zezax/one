// constant definition header for zezax::red

#pragma once

#include "Types.h"

namespace zezax::red {

enum Flags : FlagsT {
  fIgnoreCase = 0x01,
  fLooseStart = 0x02,
  fLooseEnd   = 0x04,
};

constexpr CharIdx gAlphabetSize = 256;
constexpr size_t  gNoPos        = ~0UL;
constexpr NfaId   gNfaNullId    = 0;
constexpr DfaId   gDfaErrorId   = 0;
constexpr DfaId   gDfaInitialId = 1;

} // namespace zezax::red
