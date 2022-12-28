// unit tests for nfa parser

#include <gtest/gtest.h>

#include "ReParser.h"
#include "Debug.h"

using namespace zezax::red;


TEST(Parser, smoke) {
  ReParser p;
  p.add("^a(l|e)[X-x]{,3}$", 1, 0);
  p.add("(meyer\\i)+", 2, 0);
  NfaObj &nfa = p.getNfa();
  EXPECT_NE(gNfaNullId, nfa.getNfaInitial());
  EXPECT_EQ(toString(nfa),
            R"raw(0 NfaState -> 0

1 NfaState -> 0
  2 <- a
  3 <- a
  16 <- ^@-$ff
  15 <- ^@-$ff
  17 <- ^@-$ff
  17 <- ^@-$ff
  18 <- Mm
  19 <- Mm

2 NfaState -> 0

3 NfaState -> 0
  4 <- l
  6 <- e
  7 <- l
  7 <- e
  13 <- l
  13 <- e

4 NfaState -> 0

5 NfaState -> 0
  6 <- e

6 NfaState -> 0

7 NfaState -> 0
  8 <- X-x
  9 <- X-x
  10 <- X-x
  11 <- X-x
  12 <- X-x
  13 <- X-x
  13 <- X-x
  13 <- X-x

8 NfaState -> 0

9 NfaState -> 0
  10 <- X-x
  11 <- X-x
  12 <- X-x
  13 <- X-x
  13 <- X-x

10 NfaState -> 0

11 NfaState -> 0
  12 <- X-x
  13 <- X-x

12 NfaState -> 0

13 NfaState -> 0
  14 <- [1]

14 NfaState -> 1

15 NfaState -> 0
  16 <- ^@-$ff
  15 <- ^@-$ff
  17 <- ^@-$ff
  17 <- ^@-$ff
  18 <- Mm
  19 <- Mm

16 NfaState -> 0

17 NfaState -> 0
  18 <- Mm
  19 <- Mm

18 NfaState -> 0

19 NfaState -> 0
  20 <- Ee
  21 <- Ee

20 NfaState -> 0

21 NfaState -> 0
  22 <- Yy
  23 <- Yy

22 NfaState -> 0

23 NfaState -> 0
  24 <- Ee
  25 <- Ee

24 NfaState -> 0

25 NfaState -> 0
  26 <- Rr
  27 <- Rr
  37 <- Rr
  39 <- Rr

26 NfaState -> 0

27 NfaState -> 0
  28 <- Mm
  29 <- Mm

28 NfaState -> 0

29 NfaState -> 0
  30 <- Ee
  31 <- Ee

30 NfaState -> 0

31 NfaState -> 0
  32 <- Yy
  33 <- Yy

32 NfaState -> 0

33 NfaState -> 0
  34 <- Ee
  35 <- Ee

34 NfaState -> 0

35 NfaState -> 0
  36 <- Rr
  27 <- Rr
  37 <- Rr
  37 <- Rr
  39 <- Rr
  39 <- Rr

36 NfaState -> 0

37 NfaState -> 0
  38 <- ^@-$ff
  37 <- ^@-$ff
  39 <- ^@-$ff
  39 <- ^@-$ff

38 NfaState -> 0

39 NfaState -> 0
  40 <- [2]

40 NfaState -> 2

)raw");
}
