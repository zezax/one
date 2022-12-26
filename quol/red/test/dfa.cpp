// unit tests for dfa class and utilities

#include <vector>

#include <gtest/gtest.h>

#include "Dfa.h"
#include "Debug.h" // FIXME

using namespace zezax::red;

using std::vector;

namespace {

StateId mkState(DfaObj &dfa, Result r) {
  StateId id = dfa.newState();
  dfa[id].result_ = r;
  return id;
}


void addTrans(DfaObj &dfa, StateId from, StateId to, CharIdx ch) {
  dfa[from].trans_.set(ch, to);
}

} // anonymous

TEST(Dfa, maxchar) {
  DfaObj dfa;
  StateId s0 = mkState(dfa, 0);
  StateId s1 = mkState(dfa, 0);
  StateId s2 = mkState(dfa, 0);
  StateId s3 = mkState(dfa, 0);
  StateId s4 = mkState(dfa, 1);
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

  const vector<DfaState> &vec = dfa.getMutStates();
  ASSERT_EQ(5, vec.size());
  CharIdx maxChar = dfa.findMaxChar();
  EXPECT_EQ(gAlphabetSize + 1, maxChar);
}


TEST(Dfa, equivmap) {
  DfaObj dfa;
  mkState(dfa, 0); // zero is error state
  StateId s1 = mkState(dfa, 0);
  StateId s2 = mkState(dfa, 0);
  StateId s3 = mkState(dfa, 0);
  StateId s4 = mkState(dfa, 1);
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

  const vector<DfaState> &vec = dfa.getStates();
  CharIdx maxChar = dfa.findMaxChar();
  EXPECT_EQ(gAlphabetSize + 1, maxChar);
  dfa.installEquivalenceMap();
  ASSERT_EQ(5, vec.size());
  maxChar = dfa.findMaxChar();
  EXPECT_EQ(3, maxChar); // a, b, endmark, and everything else
}