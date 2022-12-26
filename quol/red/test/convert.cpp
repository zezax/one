// unit tests for nfa-to-dfa

#include <gtest/gtest.h>

#include <vector>

#include "Util.h"
#include "NfaToDfa.h"
#include "Debug.h" // FIXME

using namespace zezax::red;

using std::vector;

namespace {

NfaState mkState(StateId id, Result result) {
  NfaState rv;
  rv.id_ = id;
  rv.result_ = result;
  rv.allocNext_ = nullptr;
  return rv;
}


void addTrans(NfaState *from, NfaState *to, CharIdx ch) {
  NfaTransition x;
  x.next_ = to;
  x.multiChar_.set(ch);
  from->transitions_.emplace_back(std::move(x));
}

} // anonymous

TEST(Convert, unique) {
  MultiChar aa('a');
  MultiChar bb('b');
  MultiChar cc('a', 'b');
  MultiChar dd('b', 'd');
  MultiChar ee('a', 'c');
  MultiCharSet mcs;
  mcs.insert(aa);
  mcs.insert(bb);
  mcs.insert(cc);
  mcs.insert(dd);
  mcs.insert(ee);
  EXPECT_EQ(5, mcs.size());
  MultiCharSet basis = basisMultiChars(mcs);
  EXPECT_EQ(4, basis.size());
  EXPECT_TRUE(basis.contains(aa));
  EXPECT_TRUE(basis.contains(bb));
  EXPECT_FALSE(basis.contains(cc));
  EXPECT_FALSE(basis.contains(dd));
  EXPECT_FALSE(basis.contains(ee));
  MultiChar yy('c');
  MultiChar zz('c', 'd');
  EXPECT_TRUE(basis.contains(yy));
  EXPECT_TRUE(basis.contains(zz));
}


TEST(Convert, stepwise) {
  // example from http://www.geeksforgeeks.org/conversion-from-nfa-to-dfa/
  // +---+
  // |a,b|
  // v   | a       b
  // S1 -+---> S2 ---> S3 accept
  NfaState s1 = mkState(1, 0);
  NfaState s2 = mkState(2, 0);
  NfaState s3 = mkState(3, 1);
  addTrans(&s1, &s1, 'a');
  addTrans(&s1, &s1, 'b');
  addTrans(&s1, &s2, 'a');
  addTrans(&s2, &s3, 'b');
  MultiCharSet all = allMultiChars(&s1);
  MultiCharSet basis = basisMultiChars(all);
  vector<MultiChar> chars;
  for (auto it = basis.begin(); it != basis.end(); ++it)
    chars.emplace_back(std::move(*it));
  NfaStatesToTransitions tbl = makeTable(&s1, chars);
  EXPECT_EQ(3, tbl.size());

  NfaStateSet ss1;
  ss1.insert(&s1);
  NfaStateSet ss12;
  ss12.insert(&s1);
  ss12.insert(&s2);
  NfaStateSet ss13;
  ss13.insert(&s1);
  ss13.insert(&s3);

  ASSERT_TRUE(tbl.contains(ss1));
  vector<NfaStateSet> &v = tbl[ss1];
  EXPECT_EQ(2, v.size());
  EXPECT_TRUE(contains(v, ss1));
  EXPECT_TRUE(contains(v, ss12));

  ASSERT_TRUE(tbl.contains(ss12));
  v = tbl[ss12];
  EXPECT_EQ(2, v.size());
  EXPECT_TRUE(contains(v, ss12));
  EXPECT_TRUE(contains(v, ss13));

  ASSERT_TRUE(tbl.contains(ss13));
  v = tbl[ss13];
  EXPECT_EQ(2, v.size());
  EXPECT_TRUE(contains(v, ss1));
  EXPECT_TRUE(contains(v, ss12));

  NfaStateToCount counts = countAcceptingStates(tbl);
  EXPECT_EQ(1, counts.size());
  ASSERT_TRUE(counts.contains(&s3));
  EXPECT_EQ(1, counts[&s3]);

  NfaStatesToId map;
  DfaObj dfa;
  StateId id = dfa.newState();
  EXPECT_EQ(gDfaErrorId, id);
  id = dfaFromNfa(chars, tbl, counts, ss1, map, dfa);
  EXPECT_EQ(gDfaInitialId, id);
  EXPECT_EQ(4, dfa.getStates().size());
  //      +---+     +---+
  //      | b |     | a |
  //      v   | a   v   | b
  // S0   S1 -+---> S2 -+---> S3 accept
}


TEST(Convert, convert) {
  // +---+
  // |a,b|
  // v   | a       b      [1]
  // S1 -+---> S2 ---> S3 ---> S4 accept
  NfaState s1 = mkState(1, 0);
  NfaState s2 = mkState(2, 0);
  NfaState s3 = mkState(3, 0);
  NfaState s4 = mkState(4, 1); // end mark
  addTrans(&s1, &s1, 'a');
  addTrans(&s1, &s1, 'b');
  addTrans(&s1, &s2, 'a');
  addTrans(&s2, &s3, 'b');
  addTrans(&s3, &s4, gAlphabetSize + 1); // end mark
  NfaObj nfa;
  nfa.setNfaInitial(&s1);

  DfaObj dfa = convertNfaToDfa(nfa);
  const vector<DfaState> &states = dfa.getStates();
  EXPECT_EQ(5, states.size());
  std::cout << toString(dfa) << std::endl;
  //      +---+     +---+
  //      | b |     | a |
  //      v   | a   v   | b
  // S0   S1 -+---> S2 -+---> S3 -+---> S4 accept
}
