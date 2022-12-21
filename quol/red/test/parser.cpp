// unit tests for nfa parser

#include <gtest/gtest.h>

#include "ReParser.h"
#include "Debug.h"

using namespace zezax::red;


TEST(Parser, smoke) {
  ReParser p;
  p.add("^a(l|e)[X-x]{,3}$", 1, 0);
  p.add("(meyer\\i)+", 2, 0);
  NfaState *nfa = p.getNfaInitial();;
  EXPECT_NE(nullptr, nfa);
  EXPECT_EQ(toStringDeep(nfa),
            R"raw(NfaState 1 -> 0
  2 <- a
  3 <- a
  16 <- ^@-$ff
  15 <- ^@-$ff
  17 <- ^@-$ff
  17 <- ^@-$ff
  18 <- Mm
  19 <- Mm

NfaState 2 -> 0

NfaState 3 -> 0
  4 <- l
  6 <- e
  7 <- l
  7 <- e
  13 <- l
  13 <- e

NfaState 4 -> 0

NfaState 6 -> 0

NfaState 7 -> 0
  8 <- X-x
  9 <- X-x
  10 <- X-x
  11 <- X-x
  12 <- X-x
  13 <- X-x
  13 <- X-x
  13 <- X-x

NfaState 8 -> 0

NfaState 9 -> 0
  10 <- X-x
  11 <- X-x
  12 <- X-x
  13 <- X-x
  13 <- X-x

NfaState 10 -> 0

NfaState 11 -> 0
  12 <- X-x
  13 <- X-x

NfaState 12 -> 0

NfaState 13 -> 0
  14 <- [1]

NfaState 14 -> 1

NfaState 15 -> 0
  16 <- ^@-$ff
  15 <- ^@-$ff
  17 <- ^@-$ff
  17 <- ^@-$ff
  18 <- Mm
  19 <- Mm

NfaState 16 -> 0

NfaState 17 -> 0
  18 <- Mm
  19 <- Mm

NfaState 18 -> 0

NfaState 19 -> 0
  20 <- Ee
  21 <- Ee

NfaState 20 -> 0

NfaState 21 -> 0
  22 <- Yy
  23 <- Yy

NfaState 22 -> 0

NfaState 23 -> 0
  24 <- Ee
  25 <- Ee

NfaState 24 -> 0

NfaState 25 -> 0
  26 <- Rr
  27 <- Rr
  37 <- Rr
  39 <- Rr

NfaState 26 -> 0

NfaState 27 -> 0
  28 <- Mm
  29 <- Mm

NfaState 28 -> 0

NfaState 29 -> 0
  30 <- Ee
  31 <- Ee

NfaState 30 -> 0

NfaState 31 -> 0
  32 <- Yy
  33 <- Yy

NfaState 32 -> 0

NfaState 33 -> 0
  34 <- Ee
  35 <- Ee

NfaState 34 -> 0

NfaState 35 -> 0
  36 <- Rr
  27 <- Rr
  37 <- Rr
  37 <- Rr
  39 <- Rr
  39 <- Rr

NfaState 36 -> 0

NfaState 37 -> 0
  38 <- ^@-$ff
  37 <- ^@-$ff
  39 <- ^@-$ff
  39 <- ^@-$ff

NfaState 38 -> 0

NfaState 39 -> 0
  40 <- [2]

NfaState 40 -> 2

)raw");
}
