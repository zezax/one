// unit tests for nfa parser

#include <gtest/gtest.h>

#include "Parser.h"
#include "Debug.h"

using namespace zezax::red;


TEST(Parser, smoke) {
  Parser p;
  p.addAuto("^a(l|e)[X-x]{,3}$", 1, 0);
  p.addAuto("(meyer\\i)+", 2, 0);
  p.finish();
  NfaObj &nfa = p.getNfa();
  EXPECT_NE(gNfaNullId, nfa.getInitial());
  EXPECT_EQ(toString(nfa),
            R"raw(1 NfaState -> 0
  3 <- a
  15 <- ^@-$ff
  17 <- ^@-$ff
  19 <- Mm

3 NfaState -> 0
  7 <- l
  7 <- e
  13 <- l
  13 <- e

7 NfaState -> 0
  9 <- X-x
  11 <- X-x
  13 <- X-x

9 NfaState -> 0
  11 <- X-x
  13 <- X-x

11 NfaState -> 0
  13 <- X-x

13 NfaState -> 0
  14 <- [1]

14 NfaState -> 1

15 NfaState -> 0
  15 <- ^@-$ff
  17 <- ^@-$ff
  19 <- Mm

17 NfaState -> 0
  19 <- Mm

19 NfaState -> 0
  21 <- Ee

21 NfaState -> 0
  23 <- Yy

23 NfaState -> 0
  25 <- Ee

25 NfaState -> 0
  27 <- Rr
  37 <- Rr
  39 <- Rr

27 NfaState -> 0
  29 <- Mm

29 NfaState -> 0
  31 <- Ee

31 NfaState -> 0
  33 <- Yy

33 NfaState -> 0
  35 <- Ee

35 NfaState -> 0
  27 <- Rr
  37 <- Rr
  39 <- Rr

37 NfaState -> 0
  37 <- ^@-$ff
  39 <- ^@-$ff

39 NfaState -> 0
  40 <- [2]

40 NfaState -> 2

)raw");
}


TEST(Parser, statebudget) {
  Budget b;
  b.initStates(3);
  Parser p(&b);
  EXPECT_THROW(p.add("ab*c", 1, 0), RedExceptLimit);
}


TEST(Parser, parenbudget) {
  Budget b;
  b.initParens(2);
  Parser p(&b);
  EXPECT_THROW(p.add("a(b(c(d)))", 1, 0), RedExceptLimit);
}
