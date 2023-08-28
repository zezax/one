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

TEST(Powerset, basis1) {
  // a
  //   b
  // a b
  //   b c d
  // a b c
  // 0 1 2 3
  MultiChar a('a');
  MultiChar b('b');
  MultiChar ab('a', 'b');
  MultiChar bcd('b', 'd');
  MultiChar abc('a', 'c');
  MultiCharSet mcs;
  mcs.insert(a);
  mcs.insert(b);
  mcs.insert(ab);
  mcs.insert(bcd);
  mcs.insert(abc);
  EXPECT_EQ(5, mcs.size());
  MultiCharSet basis = basisMultiChars(mcs);
  MultiChar c('c');
  MultiChar d('d');
  EXPECT_EQ(4, basis.size());
  EXPECT_TRUE(basis.contains(a));
  EXPECT_TRUE(basis.contains(b));
  EXPECT_TRUE(basis.contains(c));
  EXPECT_TRUE(basis.contains(d));
  EXPECT_FALSE(basis.contains(ab));
  EXPECT_FALSE(basis.contains(bcd));
  EXPECT_FALSE(basis.contains(abc));
}


TEST(Powerset, basis2) {
  // a b c
  //     c d e
  //         e f g
  // 0 0 1 2 3 4 4
  MultiChar abc('a', 'c');
  MultiChar cde('c', 'e');
  MultiChar efg('e', 'g');
  MultiCharSet mcs;
  mcs.insert(abc);
  mcs.insert(cde);
  mcs.insert(efg);
  MultiCharSet basis = basisMultiChars(mcs);
  MultiChar ab('a', 'b');
  MultiChar c('c');
  MultiChar d('c');
  MultiChar e('c');
  MultiChar fg('f', 'g');
  EXPECT_EQ(5, basis.size());
  EXPECT_TRUE(basis.contains(ab));
  EXPECT_TRUE(basis.contains(c));
  EXPECT_TRUE(basis.contains(d));
  EXPECT_TRUE(basis.contains(e));
  EXPECT_TRUE(basis.contains(fg));
  EXPECT_FALSE(basis.contains(abc));
  EXPECT_FALSE(basis.contains(cde));
  EXPECT_FALSE(basis.contains(efg));
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
  nfa.setInitial(s1);
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

  NfaIdToCount counts = countAcceptingStates(tbl, nfa);
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
  nfa.setInitial(s1);

  DfaObj dfa;
  {
    CompStats stats;
    PowersetConverter psc(nfa, nullptr, &stats);
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
