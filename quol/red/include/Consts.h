/* Consts.h - constant definition header for zezax::red

   This contains definitions that are generally applicable across the project.
 */

#pragma once

#include "Types.h"

namespace zezax::red {

enum FlagsE : Flags {
  fIgnoreCase = 0x01,
  fLooseStart = 0x02,
  fLooseEnd   = 0x04,
};

constexpr CharIdx gAlphabetSize = 256;
constexpr size_t  gNoPos        = ~0UL;
constexpr NfaId   gNfaNullId    = 0;
constexpr DfaId   gDfaErrorId   = 0;
constexpr DfaId   gDfaInitialId = 1;

#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

} // namespace zezax::red
