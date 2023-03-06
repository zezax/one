// unit tests for nfa object

#include <gtest/gtest.h>

#include "Nfa.h"

using namespace zezax::red;

namespace {

void addTrans(NfaObj &nfa, NfaId from, NfaId to, CharIdx ch) {
  NfaTransition x;
  x.next_ = to;
  x.multiChar_.insert(ch);
  nfa[from].transitions_.emplace_back(std::move(x));
}

} // anonymous

TEST(Nfa, smoke) {
  NfaObj nfa;
  NfaId s1 = nfa.newState(0);
  NfaId s2 = nfa.newState(0);
  NfaId s3 = nfa.newState(1);
  addTrans(nfa, s1, s1, 'a');
  addTrans(nfa, s1, s1, 'b');
  addTrans(nfa, s1, s2, 'a');
  addTrans(nfa, s2, s3, 'b');
  nfa.setInitial(s1);
  nfa.setGoal(s3);

  EXPECT_EQ(4, nfa.numStates());
  EXPECT_EQ(3, nfa.activeStates());
  EXPECT_EQ(s1, nfa.getInitial());
  EXPECT_EQ(s3, nfa.getGoal());
  EXPECT_FALSE(nfa.accepts(s1));
  EXPECT_FALSE(nfa.accepts(s2));
  EXPECT_TRUE(nfa.accepts(s3));

  NfaObj two(std::move(nfa));
  EXPECT_EQ(4, two.numStates());
  EXPECT_EQ(3, two.activeStates());
  EXPECT_EQ(s1, two.getInitial());
  EXPECT_EQ(s3, two.getGoal());
  EXPECT_FALSE(two.accepts(s1));
  EXPECT_FALSE(two.accepts(s2));
  EXPECT_TRUE(two.accepts(s3));

  NfaObj three;
  three = std::move(two);
  EXPECT_EQ(4, three.numStates());
  EXPECT_EQ(3, three.activeStates());
  EXPECT_EQ(s1, three.getInitial());
  EXPECT_EQ(s3, three.getGoal());
  EXPECT_FALSE(three.accepts(s1));
  EXPECT_FALSE(three.accepts(s2));
  EXPECT_TRUE(three.accepts(s3));
}


TEST(Nfa, iter) {
  NfaObj nfa;
  /* s1 = */ nfa.newState(0);
  NfaId s2 = nfa.newState(0);
  NfaId s3 = nfa.newState(0);
  /* s4 = */ nfa.newState(0);
  NfaId s5 = nfa.newState(1);
  addTrans(nfa, s2, s2, 'a');
  addTrans(nfa, s2, s2, 'b');
  addTrans(nfa, s2, s3, 'a');
  addTrans(nfa, s3, s5, 'b');
  NfaId sum = 0;
  NfaIter it = nfa.iter(s2);
  for (; it; ++it)
    sum += 100 + it.id();
  EXPECT_EQ(310, sum);
  EXPECT_EQ(3, it.seen().size());
}


TEST(Nfa, constiter) {
  NfaObj nfa;
  /* s1 = */ nfa.newState(0);
  NfaId s2 = nfa.newState(0);
  NfaId s3 = nfa.newState(0);
  /* s4 = */ nfa.newState(0);
  NfaId s5 = nfa.newState(1);
  addTrans(nfa, s2, s2, 'a');
  addTrans(nfa, s2, s2, 'b');
  addTrans(nfa, s2, s3, 'a');
  addTrans(nfa, s3, s5, 'b');
  NfaId sum = 0;
  NfaConstIter it = nfa.citer(s2);
  for (; it; ++it)
    sum += 100 + it.id();
  EXPECT_EQ(310, sum);
  EXPECT_EQ(3, it.seen().size());
}
