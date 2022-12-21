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
  EXPECT_EQ(gDfaErrorId, s0);
  EXPECT_EQ(gDfaInitialId, s1);
  std::cout << toString(dfa) << std::endl;
  //      +---+     +---+
  //      | b |     | a |
  //      v   | a   v   | b
  // S0   S1 -+---> S2 -+---> S3 -+---> S4 accept

  vector<DfaState> &vec = dfa.getMutStates();
  ASSERT_EQ(5, vec.size());
  CharIdx maxChar = findMaxChar(vec);
  EXPECT_EQ(gAlphabetSize + 1, maxChar);

  EXPECT_EQ(0, vec[s3].result_);
  EXPECT_EQ(1, vec[s4].result_);
  pullBackEndMarks(vec);
  EXPECT_EQ(1, vec[s3].result_);
  EXPECT_EQ(1, vec[s4].result_);
  std::cout << toString(dfa) << std::endl;

  StateIdSet states = dfa.allStateIds();
  EXPECT_EQ(5, states.population());
  DfaEdgeToIds rev = invert(states, vec, maxChar);
  EXPECT_EQ(262, rev.size()); // FIXME: 5
  std::cout << toString(rev) << std::endl;

  vector<StateIdSet> blocks;
  blocks.resize(2);
  StateIdSet &normal = blocks[0];
  StateIdSet &accept = blocks[1];
  partition(states, vec, normal, accept);
  ASSERT_EQ(3, normal.population());
  EXPECT_TRUE(normal.get(s0));
  EXPECT_TRUE(normal.get(s1));
  EXPECT_TRUE(normal.get(s2));
  ASSERT_EQ(2, accept.population());
  EXPECT_TRUE(accept.get(s3));
  EXPECT_TRUE(accept.get(s4));

  BlockRecSet list = makeList(maxChar, normal, accept);
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

  DfaMinimizer dm(dfa);
  dm.minimize();

  std::cout << "FIXME final " << toString(dfa) << std::endl;

  EXPECT_EQ(0, dfa.match(""));
  EXPECT_EQ(1, dfa.match("ab"));
  EXPECT_EQ(0, dfa.match("aba"));
  EXPECT_EQ(1, dfa.match("baab"));
}
