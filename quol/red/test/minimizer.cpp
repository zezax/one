// unit tests for dfa minimizer

#include <gtest/gtest.h>

#include <vector>

#include "Util.h"
#include "Minimizer.h"
#include "Debug.h" // FIXME

using namespace zezax::red;

using std::vector;

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

TEST(Minimizer, invert) {
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
  std::cout << toString(dfa) << std::endl;
  //      +---+     +---+
  //      | b |     | a |
  //      v   | a   v   | b
  // S0   S1 -+---> S2 -+---> S3 -+---> S4 accept

  CharIdx maxChar = dfa.findMaxChar();

  const vector<DfaState> &vec = dfa.getStates();
  EXPECT_EQ(0, vec[s3].result_);
  EXPECT_EQ(1, vec[s4].result_);
  dfa.chopEndMarks();
  EXPECT_EQ(1, vec[s3].result_);

  dfa.installEquivalenceMap();
  maxChar = dfa.findMaxChar();
  std::cout << toString(dfa) << std::endl;

  DfaIdSet states = dfa.allStateIds();
  EXPECT_EQ(4, states.size());
  DfaEdgeToIds rev = invert(states, vec, maxChar);
  EXPECT_EQ(6, rev.size());
  std::cout << toString(rev) << std::endl;

  vector<DfaIdSet> blocks;
  partition(states, vec, blocks);
  ASSERT_EQ(2, blocks.size());
  DfaIdSet &normal = blocks[0];
  DfaIdSet &accept = blocks[1];
  ASSERT_EQ(3, normal.size());
  EXPECT_TRUE(normal.get(s0));
  EXPECT_TRUE(normal.get(s1));
  EXPECT_TRUE(normal.get(s2));
  ASSERT_EQ(1, accept.size());
  EXPECT_TRUE(accept.get(s3));

  BlockRecSet list = makeList(maxChar, blocks);
  std::cout << toString(list) << std::endl;

  BlockRec br;
  br.block_ = s1; // using state id as block id
  br.char_ = 'b';
  DfaIdSet splits = locateSplits(br, blocks, rev);
  std::cout << toString(splits) << std::endl;
}


TEST(Minimizer, obj) {
  //      +---+     +---+
  //      | b |     | a |
  //      v   | a   v   | b
  // S0   S1 -+---> S2 -+---> S3 -+---> S4 accept
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

  dfa.chopEndMarks();
  {
    DfaMinimizer dm(dfa);
    dm.minimize();
  }

  std::cout << toString(dfa) << std::endl;

  EXPECT_EQ(0, dfa.matchFull(""));
  EXPECT_EQ(1, dfa.matchFull("ab"));
  EXPECT_EQ(0, dfa.matchFull("aba"));
  EXPECT_EQ(1, dfa.matchFull("baab"));
}


TEST(Minimizer, results) {
  DfaObj dfa;
  DfaId s0 = mkState(dfa, 0);
  DfaId s1 = mkState(dfa, 0);
  DfaId s2 = mkState(dfa, 0);
  DfaId s3 = mkState(dfa, 2);
  DfaId s4 = mkState(dfa, 2);
  DfaId s5 = mkState(dfa, 0);
  DfaId s6 = mkState(dfa, 1);
  DfaId s7 = mkState(dfa, 1);
  addTrans(dfa, s1, s2, 'c');
  addTrans(dfa, s1, s5, 'a');
  addTrans(dfa, s2, s3, 'b');
  addTrans(dfa, s5, s6, 'c');
  (void) s0;
  (void) s4;
  (void) s7;
  dfa.chopEndMarks();
  EXPECT_EQ(0, dfa.matchFull("ab"));
  EXPECT_EQ(1, dfa.matchFull("ac"));
  EXPECT_EQ(2, dfa.matchFull("cb"));
  {
    DfaMinimizer dm(dfa);
    dm.minimize();
  }
  EXPECT_EQ(0, dfa.matchFull("ab"));
  EXPECT_EQ(1, dfa.matchFull("ac"));
  EXPECT_EQ(2, dfa.matchFull("cb"));
}


TEST(Minimizer, deadEnds) {
  DfaObj dfa;
  /* s0 = */ mkState(dfa, 0);
  DfaId s1 = mkState(dfa, 0);
  DfaId s2 = mkState(dfa, 0);
  DfaId s3 = mkState(dfa, 1);
  addTrans(dfa, s1, s1, 0);
  addTrans(dfa, s1, s1, 2);
  addTrans(dfa, s1, s1, 3);
  addTrans(dfa, s1, s2, 1);
  addTrans(dfa, s2, s1, 0);
  addTrans(dfa, s2, s2, 1);
  addTrans(dfa, s2, s2, 2);
  addTrans(dfa, s2, s3, 3);
  addTrans(dfa, s3, s3, 0);
  addTrans(dfa, s3, s3, 1);
  addTrans(dfa, s3, s3, 2);
  addTrans(dfa, s3, s3, 3);
  dfa.chopEndMarks();
  {
    DfaMinimizer dm(dfa);
    dm.minimize();
  }
  ASSERT_EQ(4, dfa.numStates());
  // minimization has mostly renumbered the states, so...
  int live = 0;
  int dead = 0;
  for (DfaId ii = 0; ii < 4; ++ii) {
    if (dfa[ii].deadEnd_)
      ++dead;
    else
      ++live;
  }
  EXPECT_EQ(2, live);
  EXPECT_EQ(2, dead);
  EXPECT_TRUE(dfa[0].deadEnd_);
}
