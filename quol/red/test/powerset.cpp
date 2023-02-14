// unit tests for nfa-to-dfa

#include <gtest/gtest.h>

#include <vector>

#include "Util.h"
#include "Powerset.h"
#include "Debug.h" // FIXME

using namespace zezax::red;

using std::vector;

namespace {

void addTrans(NfaObj &nfa, NfaId from, NfaId to, CharIdx ch) {
  NfaTransition x;
  x.next_ = to;
  x.multiChar_.insert(ch);
  nfa[from].transitions_.emplace_back(std::move(x));
}

} // anonymous

TEST(Powerset, unique) {
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


TEST(Powerset, stepwise) {
  // example from http://www.geeksforgeeks.org/conversion-from-nfa-to-dfa/
  // +---+
  // |a,b|
  // v   | a       b
  // S1 -+---> S2 ---> S3 accept
  NfaObj nfa;
  NfaId s1 = nfa.newState(0);
  NfaId s2 = nfa.newState(0);
  NfaId s3 = nfa.newState(1);
  addTrans(nfa, s1, s1, 'a');
  addTrans(nfa, s1, s1, 'b');
  addTrans(nfa, s1, s2, 'a');
  addTrans(nfa, s2, s3, 'b');
  nfa.setNfaInitial(s1);
  MultiCharSet all = nfa.allMultiChars(s1);
  MultiCharSet basis = basisMultiChars(all);
  vector<MultiChar> chars;
  for (const MultiChar &mc : basis)
    chars.emplace_back(std::move(mc));
  NfaStatesToTransitions tbl = makeTable(s1, nfa, chars);
  EXPECT_EQ(3, tbl.size());

  NfaIdSet nis1;
  nis1.insert(s1);
  NfaIdSet nis12;
  nis12.insert(s1);
  nis12.insert(s2);
  NfaIdSet nis13;
  nis13.insert(s1);
  nis13.insert(s3);

  ASSERT_TRUE(tbl.contains(nis1));
  IdxToNfaIdSet &v = tbl[nis1];
  EXPECT_EQ(2, v.size());
  EXPECT_TRUE(contains(v, nis1));
  EXPECT_TRUE(contains(v, nis12));

  ASSERT_TRUE(tbl.contains(nis12));
  v = tbl[nis12];
  EXPECT_EQ(2, v.size());
  EXPECT_TRUE(contains(v, nis12));
  EXPECT_TRUE(contains(v, nis13));

  ASSERT_TRUE(tbl.contains(nis13));
  v = tbl[nis13];
  EXPECT_EQ(2, v.size());
  EXPECT_TRUE(contains(v, nis1));
  EXPECT_TRUE(contains(v, nis12));

  NfaStateToCount counts = countAcceptingStates(tbl, nfa);
  EXPECT_EQ(1, counts.size());
  ASSERT_TRUE(counts.contains(s3));
  EXPECT_EQ(1, counts[s3]);

  DfaObj dfa;
  DfaId id = dfa.newState();
  EXPECT_EQ(gDfaErrorId, id);
  {
    NfaStatesToId map;
    id = dfaFromNfaRecurse(chars, tbl, counts, nis1, map, nfa, dfa);
  }
  EXPECT_EQ(gDfaInitialId, id);
  EXPECT_EQ(4, dfa.numStates());
  //      +---+     +---+
  //      | b |     | a |
  //      v   | a   v   | b
  // S0   S1 -+---> S2 -+---> S3 accept
}


TEST(Powerset, convert) {
  // +---+
  // |a,b|
  // v   | a       b      [1]
  // S1 -+---> S2 ---> S3 ---> S4 accept
  NfaObj nfa;
  NfaId s1 = nfa.newState(0);
  NfaId s2 = nfa.newState(0);
  NfaId s3 = nfa.newState(0);
  NfaId s4 = nfa.newState(1); // end mark
  addTrans(nfa, s1, s1, 'a');
  addTrans(nfa, s1, s1, 'b');
  addTrans(nfa, s1, s2, 'a');
  addTrans(nfa, s2, s3, 'b');
  addTrans(nfa, s3, s4, gAlphabetSize + 1); // end mark
  nfa.setNfaInitial(s1);

  DfaObj dfa;
  {
    CompStats stats;
    PowersetConverter psc(nfa, &stats);
    dfa = psc.convert();
    nfa.freeAll();
  }
  const vector<DfaState> &states = dfa.getStates();
  EXPECT_EQ(5, states.size());
  std::cout << toString(dfa) << std::endl;
  //      +---+     +---+
  //      | b |     | a |
  //      v   | a   v   | b
  // S0   S1 -+---> S2 -+---> S3 -+---> S4 accept
}
