// unit tests for dfa class and utilities

#include <gtest/gtest.h>

#include "Dfa.h"

using namespace zezax::red;

namespace {

DfaId mkState(DfaObj &dfa, Result r) {
  DfaId id = dfa.newState();
  dfa[id].result_ = r;
  return id;
}


void addTrans(DfaObj &dfa, DfaId from, DfaId to, CharIdx ch) {
  dfa[from].trans_.set(ch, to);
}

} // anonymous

TEST(Dfa, maxchar) {
  DfaObj dfa;
  DfaId s0 = mkState(dfa, 0);
  DfaId s1 = mkState(dfa, 0);
  DfaId s2 = mkState(dfa, 0);
  DfaId s3 = mkState(dfa, 0);
  DfaId s4 = mkState(dfa, 1);
  addTrans(dfa, s1, s1, 'b');
  addTrans(dfa, s1, s2, 'a');
  addTrans(dfa, s2, s2, 'a');
  addTrans(dfa, s2, s3, 'b');
  addTrans(dfa, s3, s4, gAlphabetSize + 1); // end mark
  // FIXME: the actual powerset conversion creates extra states from s3
  EXPECT_EQ(gDfaErrorId, s0);
  EXPECT_EQ(gDfaInitialId, s1);
  //      +---+     +---+
  //      | b |     | a |
  //      v   | a   v   | b
  // S0   S1 -+---> S2 -+---> S3 -+---> S4 accept

  ASSERT_EQ(5, dfa.numStates());
  CharIdx maxChar = dfa.findMaxChar();
  EXPECT_EQ(gAlphabetSize + 1, maxChar);

  MultiChar mc;
  CharIdx mx = dfa.findUsedChars(mc);
  EXPECT_EQ(mx, maxChar);
  EXPECT_EQ(3, mc.size()); // a, b, endmark
  EXPECT_TRUE(mc.get('a'));
  EXPECT_TRUE(mc.get('b'));
  EXPECT_TRUE(mc.get(gAlphabetSize + 1));
}


TEST(Dfa, maxresult) {
  DfaObj dfa;
  DfaId s0 = mkState(dfa, 0);
  DfaId s1 = mkState(dfa, 1);
  DfaId s2 = mkState(dfa, 2);
  DfaId s3 = mkState(dfa, 3);
  DfaId s4 = mkState(dfa, 4);
  addTrans(dfa, s1, s1, 'b');
  addTrans(dfa, s1, s2, 'a');
  addTrans(dfa, s2, s2, 'a');
  addTrans(dfa, s2, s3, 'b');
  addTrans(dfa, s3, s4, gAlphabetSize + 1); // end mark
  EXPECT_EQ(gDfaErrorId, s0);
  EXPECT_EQ(gDfaInitialId, s1);
  Result maxResult = dfa.findMaxResult();
  EXPECT_EQ(4, maxResult);
}


TEST(Dfa, equivmap) {
  DfaObj dfa;
  mkState(dfa, 0); // zero is error state
  DfaId s1 = mkState(dfa, 0);
  DfaId s2 = mkState(dfa, 0);
  DfaId s3 = mkState(dfa, 0);
  DfaId s4 = mkState(dfa, 1);
  addTrans(dfa, s1, s1, 'b');
  addTrans(dfa, s1, s2, 'a');
  addTrans(dfa, s2, s2, 'a');
  addTrans(dfa, s2, s3, 'b');
  addTrans(dfa, s3, s4, gAlphabetSize + 1); // end mark
  // FIXME: the actual powerset conversion creates extra states from s3
  //      +---+     +---+
  //      | b |     | a |
  //      v   | a   v   | b
  // S0   S1 -+---> S2 -+---> S3 -+---> S4 accept

  CharIdx maxChar = dfa.findMaxChar();
  EXPECT_EQ(gAlphabetSize + 1, maxChar);
  CharIdx mx = dfa.installEquivalenceMap();
  EXPECT_EQ(3, mx); // a, b, the rest, endmark
  maxChar = dfa.findMaxChar();
  EXPECT_GE(mx, maxChar);
  EXPECT_LE(mx, maxChar + 1); // !!! depends on which states are reachable
}


TEST(Dfa, equivmapChop) {
  DfaObj dfa;
  mkState(dfa, 0); // zero is error state
  DfaId s1 = mkState(dfa, 0);
  DfaId s2 = mkState(dfa, 0);
  DfaId s3 = mkState(dfa, 0);
  DfaId s4 = mkState(dfa, 1);
  addTrans(dfa, s1, s1, 'b');
  addTrans(dfa, s1, s2, 'a');
  addTrans(dfa, s2, s2, 'a');
  addTrans(dfa, s2, s3, 'b');
  addTrans(dfa, s3, s4, gAlphabetSize + 1); // end mark
  // FIXME: the actual powerset conversion creates extra states from s3
  //      +---+     +---+
  //      | b |     | a |
  //      v   | a   v   | b
  // S0   S1 -+---> S2 -+---> S3 -+---> S4 accept

  dfa.chopEndMarks();
  CharIdx maxChar = dfa.findMaxChar();
  EXPECT_EQ('b', maxChar);
  CharIdx mx = dfa.installEquivalenceMap();
  EXPECT_EQ(2, mx); // a, b, the rest
  maxChar = dfa.findMaxChar();
  EXPECT_GE(mx, maxChar);
  EXPECT_LE(mx, maxChar + 1); // !!! depends on which states are reachable
}


TEST(Dfa, equivmapZero) {
  DfaObj dfa;
  mkState(dfa, 0); // need at least two states to be valid (now)
  mkState(dfa, 0);
  CharIdx maxChar = dfa.findMaxChar();
  EXPECT_EQ(0, maxChar);
  CharIdx mx = dfa.installEquivalenceMap();
  maxChar = dfa.findMaxChar();
  EXPECT_EQ(0, maxChar);
  EXPECT_EQ(0, mx);
}


TEST(Dfa, equivmapAll) {
  DfaObj dfa;
  /* s0 = */ mkState(dfa, 0);
  DfaId s1 = mkState(dfa, 0);
  DfaId s2 = mkState(dfa, 0);
  DfaId s3 = mkState(dfa, 0);
  DfaId s4 = mkState(dfa, 1);
  DfaId s5 = mkState(dfa, 2);
  addTrans(dfa, s2, s4, gAlphabetSize + 1); // end mark
  addTrans(dfa, s3, s5, gAlphabetSize + 2); // end mark
  for (CharIdx ch = 0; ch < gAlphabetSize; ++ch)
    if (ch & 1)
      addTrans(dfa, s1, s3, ch);
    else
      addTrans(dfa, s1, s2, ch);
  CharIdx maxChar = dfa.findMaxChar();
  EXPECT_EQ(gAlphabetSize + 2, maxChar);
  CharIdx mx = dfa.installEquivalenceMap();
  EXPECT_EQ(4, mx); // 2 equivalence classes, 1 in-between, 2 endmarks
  maxChar = dfa.findMaxChar();
  EXPECT_GE(mx, maxChar);
  EXPECT_LE(mx, maxChar + 1); // !!! depends on which states are reachable
}


TEST(Dfa, equivmapAllChop) {
  DfaObj dfa;
  /* s0 = */ mkState(dfa, 0);
  DfaId s1 = mkState(dfa, 0);
  DfaId s2 = mkState(dfa, 0);
  DfaId s3 = mkState(dfa, 0);
  DfaId s4 = mkState(dfa, 1);
  DfaId s5 = mkState(dfa, 2);
  addTrans(dfa, s2, s4, gAlphabetSize + 1); // end mark
  addTrans(dfa, s3, s5, gAlphabetSize + 2); // end mark
  for (CharIdx ch = 0; ch < gAlphabetSize; ++ch)
    if (ch & 1)
      addTrans(dfa, s1, s3, ch);
    else
      addTrans(dfa, s1, s2, ch);
  dfa.chopEndMarks();
  CharIdx maxChar = dfa.findMaxChar();
  EXPECT_EQ(gAlphabetSize - 1, maxChar);
  CharIdx mx = dfa.installEquivalenceMap();
  maxChar = dfa.findMaxChar();
  EXPECT_EQ(1, maxChar); // 2 equivalence classes
  EXPECT_GE(mx, maxChar);
  EXPECT_LE(mx, maxChar + 1); // !!! depends on which states are reachable
}


TEST(Dfa, equivmapHalf) {
  DfaObj dfa;
  /* s0 = */ mkState(dfa, 0);
  DfaId s1 = mkState(dfa, 0);
  DfaId s2 = mkState(dfa, 0);
  DfaId s3 = mkState(dfa, 0);
  DfaId s4 = mkState(dfa, 1);
  DfaId s5 = mkState(dfa, 2);
  addTrans(dfa, s2, s4, gAlphabetSize + 1); // end mark
  addTrans(dfa, s3, s5, gAlphabetSize + 2); // end mark
  for (CharIdx ch = 0; ch < 128; ++ch)
    if (ch & 1)
      addTrans(dfa, s1, s3, ch);
    else
      addTrans(dfa, s1, s2, ch);
  CharIdx maxChar = dfa.findMaxChar();
  EXPECT_EQ(gAlphabetSize + 2, maxChar);
  CharIdx mx = dfa.installEquivalenceMap();
  EXPECT_EQ(4, mx); // evens, odds, high-ascii, 2 endmarks
  maxChar = dfa.findMaxChar();
  EXPECT_GE(mx, maxChar);
  EXPECT_LE(mx, maxChar + 1); // !!! depends on which states are reachable
}


TEST(Dfa, equivmapHalfChop) {
  DfaObj dfa;
  /* s0 = */ mkState(dfa, 0);
  DfaId s1 = mkState(dfa, 0);
  DfaId s2 = mkState(dfa, 0);
  DfaId s3 = mkState(dfa, 0);
  DfaId s4 = mkState(dfa, 1);
  DfaId s5 = mkState(dfa, 2);
  addTrans(dfa, s2, s4, gAlphabetSize + 1); // end mark
  addTrans(dfa, s3, s5, gAlphabetSize + 2); // end mark
  for (CharIdx ch = 0; ch < 128; ++ch)
    if (ch & 1)
      addTrans(dfa, s1, s3, ch);
    else
      addTrans(dfa, s1, s2, ch);
  dfa.chopEndMarks();
  CharIdx maxChar = dfa.findMaxChar();
  EXPECT_EQ(127, maxChar);
  CharIdx mx = dfa.installEquivalenceMap();
  EXPECT_EQ(2, mx); // evens, odds, high-ascii
  maxChar = dfa.findMaxChar();
  EXPECT_GE(mx, maxChar);
  EXPECT_LE(mx, maxChar + 1); // !!! depends on which states are reachable
}


TEST(Dfa, iter) {
  DfaObj dfa;
  mkState(dfa, 0);
  DfaId s1 = mkState(dfa, 0);
  DfaId s2 = mkState(dfa, 0);
  mkState(dfa, 0);
  DfaId s4 = mkState(dfa, 0);
  DfaId s5 = mkState(dfa, 1);
  addTrans(dfa, s1, s1, 'b');
  addTrans(dfa, s1, s2, 'a');
  addTrans(dfa, s2, s2, 'a');
  addTrans(dfa, s2, s4, 'b');
  addTrans(dfa, s4, s5, gAlphabetSize + 1);
  EXPECT_EQ(6, dfa.numStates());
  DfaId sum = 0;
  DfaIter it = dfa.iter();
  for (; it; ++it)
    sum += 100 + it.id();
  EXPECT_EQ(512, sum); // 0 + 1 + 2 + 4 + 5 = 12
  EXPECT_EQ(5, it.seen().size());
  EXPECT_TRUE(it.seen().get(0));
  EXPECT_TRUE(it.seen().get(1));
  EXPECT_TRUE(it.seen().get(2));
  EXPECT_FALSE(it.seen().get(3));
  EXPECT_TRUE(it.seen().get(4));
  EXPECT_TRUE(it.seen().get(5));
}


TEST(Dfa, constiter) {
  DfaObj dfa;
  mkState(dfa, 0);
  DfaId s1 = mkState(dfa, 0);
  DfaId s2 = mkState(dfa, 0);
  mkState(dfa, 0);
  DfaId s4 = mkState(dfa, 0);
  DfaId s5 = mkState(dfa, 1);
  addTrans(dfa, s1, s1, 'b');
  addTrans(dfa, s1, s2, 'a');
  addTrans(dfa, s2, s2, 'a');
  addTrans(dfa, s2, s4, 'b');
  addTrans(dfa, s4, s5, gAlphabetSize + 1);
  EXPECT_EQ(6, dfa.numStates());
  DfaId sum = 0;
  DfaConstIter it = dfa.citer();
  for (; it; ++it)
    sum += 100 + it.id();
  EXPECT_EQ(512, sum); // 0 + 1 + 2 + 4 + 5 = 12
  EXPECT_EQ(5, it.seen().size());
  EXPECT_TRUE(it.seen().get(0));
  EXPECT_TRUE(it.seen().get(1));
  EXPECT_TRUE(it.seen().get(2));
  EXPECT_FALSE(it.seen().get(3));
  EXPECT_TRUE(it.seen().get(4));
  EXPECT_TRUE(it.seen().get(5));
}
