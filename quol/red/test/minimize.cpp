// unit tests for dfa minimizer

#include <gtest/gtest.h>

#include <vector>

#include "Util.h"
#include "DfaMinimizer.h"
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

TEST(Minimize, invert) {
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

  StateIdSet states = dfa.allStateIds();
  EXPECT_EQ(4, states.population());
  DfaEdgeToIds rev = invert(states, vec, maxChar);
  EXPECT_EQ(6, rev.size());
  std::cout << toString(rev) << std::endl;

  vector<StateIdSet> blocks;
  partition(states, vec, blocks);
  ASSERT_EQ(2, blocks.size());
  StateIdSet &normal = blocks[0];
  StateIdSet &accept = blocks[1];
  ASSERT_EQ(3, normal.population());
  EXPECT_TRUE(normal.get(s0));
  EXPECT_TRUE(normal.get(s1));
  EXPECT_TRUE(normal.get(s2));
  ASSERT_EQ(1, accept.population());
  EXPECT_TRUE(accept.get(s3));

  BlockRecSet list = makeList(maxChar, blocks);
  std::cout << toString(list) << std::endl;

  BlockRec br;
  br.block_ = s1; // using state id as block id
  br.char_ = 'b';
  StateIdSet splits = locateSplits(br, blocks, rev);
  std::cout << toString(splits) << std::endl;
}


TEST(Minimize, obj) {
  //      +---+     +---+
  //      | b |     | a |
  //      v   | a   v   | b
  // S0   S1 -+---> S2 -+---> S3 -+---> S4 accept
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

  dfa.chopEndMarks();
  {
    DfaMinimizer dm(dfa);
    dm.minimize();
  }

  std::cout << "FIXME final " << toString(dfa) << std::endl;

  EXPECT_EQ(0, dfa.matchWhole(""));
  EXPECT_EQ(1, dfa.matchWhole("ab"));
  EXPECT_EQ(0, dfa.matchWhole("aba"));
  EXPECT_EQ(1, dfa.matchWhole("baab"));
}


TEST(Minimize, results) {
  DfaObj dfa;
  StateId s0 = mkState(dfa, 0);
  StateId s1 = mkState(dfa, 0);
  StateId s2 = mkState(dfa, 0);
  StateId s3 = mkState(dfa, 2);
  StateId s4 = mkState(dfa, 2);
  StateId s5 = mkState(dfa, 0);
  StateId s6 = mkState(dfa, 1);
  StateId s7 = mkState(dfa, 1);
  addTrans(dfa, s1, s2, 'c');
  addTrans(dfa, s1, s5, 'a');
  addTrans(dfa, s2, s3, 'b');
  addTrans(dfa, s5, s6, 'c');
  (void) s0;
  (void) s4;
  (void) s7;
  dfa.chopEndMarks();
  EXPECT_EQ(0, dfa.matchWhole("ab"));
  EXPECT_EQ(1, dfa.matchWhole("ac"));
  EXPECT_EQ(2, dfa.matchWhole("cb"));
  {
    DfaMinimizer dm(dfa);
    dm.minimize();
  }
  EXPECT_EQ(0, dfa.matchWhole("ab"));
  EXPECT_EQ(1, dfa.matchWhole("ac"));
  EXPECT_EQ(2, dfa.matchWhole("cb"));
}


TEST(Minimize, deadEnds) {
  DfaObj dfa;
  StateId s0 = mkState(dfa, 0);
  StateId s1 = mkState(dfa, 0);
  StateId s2 = mkState(dfa, 0);
  StateId s3 = mkState(dfa, 1);
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
  EXPECT_TRUE(dfa[s0].deadEnd_);
  EXPECT_FALSE(dfa[s1].deadEnd_);
  EXPECT_TRUE(dfa[s2].deadEnd_);
  EXPECT_FALSE(dfa[s3].deadEnd_);
}
