// unit tests for utilities

#include <vector>

#include <gtest/gtest.h>

#include "Except.h"
#include "Debug.h"

using namespace zezax::red;

using std::vector;


TEST(Debug, toHexDigit) {
  EXPECT_EQ('0', toHexDigit(0));
  EXPECT_EQ('9', toHexDigit(9));
  EXPECT_EQ('a', toHexDigit(10));
  EXPECT_EQ('f', toHexDigit(15));
  EXPECT_THROW(toHexDigit(16), RedExceptInternal);
}


TEST(Debug, toHexString) {
  EXPECT_EQ("0", toHexString(0));
  EXPECT_EQ("9", toHexString(9));
  EXPECT_EQ("a", toHexString(10));
  EXPECT_EQ("f", toHexString(15));
  EXPECT_EQ("ff", toHexString(255));
  EXPECT_EQ("ffff", toHexString(65535));
  EXPECT_EQ("ffffffff", toHexString(-1));
}


TEST(Debug, multichar) {
  MultiChar mc('a', 'c');
  EXPECT_EQ("a-c", toString(mc));

  MultiCharSet mcs;
  mcs.emplace(std::move(mc));
  mcs.emplace('x', 'z');
  EXPECT_EQ("a-c\nx-z\n", toString(mcs));
}


TEST(Debug, idsets) {
  NfaIdSet nis;
  nis.set(3);
  nis.set(4);
  nis.set(5);
  EXPECT_EQ("3-5", toString(nis));
  DfaIdSet dis;
  dis.set(0);
  dis.set(8);
  dis.set(6);
  dis.set(9);
  EXPECT_EQ("0,6,8-9", toString(dis));
}


TEST(Debug, blocks) {
  vector<DfaIdSet> b;
  b.emplace_back(3);
  b.emplace_back(7, 9);
  EXPECT_EQ("blocks[\n  0: 3\n  1: 7-9\n]\n", toString(b));
}


#pragma GCC diagnostic ignored "-Wconversion"

TEST(Debug, token) {
  TokEnum bad = static_cast<TokEnum>(666);
  Token t0(bad, gNoPos);
  EXPECT_THROW(toString(t0), RedExceptInternal);
  Token t1(tError, 1);
  EXPECT_EQ("error", toString(t1));
  Token t2(tEnd, 2);
  EXPECT_EQ("end", toString(t2));
  Token t3(gTokFlag, fIgnoreCase, 3);
  EXPECT_EQ("flags 1", toString(t3));
  Token t4(tChars, 4);
  t4.multiChar_.set('a');
  EXPECT_EQ("chars a", toString(t4));
  Token t5(5, 0, 1);
  EXPECT_EQ("close 0:1", toString(t5));
  Token t6(tUnion, 6);
  EXPECT_EQ("union", toString(t6));
  Token t7(tLeft, 7);
  EXPECT_EQ("left", toString(t7));
  Token t8(tRight, 8);
  EXPECT_EQ("right", toString(t8));
}


TEST(Debug, nfa) {
  NfaObj nfa;
  NfaId one = nfa.newState(1);
  EXPECT_EQ(1, one);
  nfa.setInitial(one);
  NfaTransition tr;
  tr.next_ = one;
  tr.multiChar_.set('a');
  nfa[one].transitions_.emplace_back(std::move(tr));
  EXPECT_EQ("1 NfaState -> 1\n  1 <- a\n\n", toString(nfa));

  NfaIdSet nis;
  nis.set(1);
  EXPECT_EQ("set{\n1 NfaState -> 1\n  1 <- a\n\n}\n", toString(nis, nfa));
}


TEST(Debug, nstt) {
  NfaStatesToTransitions tbl;
  NfaIdSet key(2, 4);
  IdxToNfaIdSet val;
  val[1] = NfaIdSet(1,2);
  val[3] = NfaIdSet(5);
  tbl[key] = val;
  EXPECT_EQ("map(\nkey=2-4 val=[1-2;5]\n)\n", toString(tbl));
}


TEST(Debug, nstc) {
  NfaIdToCount tbl;
  tbl[1] = 13;
  tbl[3] = 69;
  EXPECT_EQ("counts(\nkey=1 val=13\nkey=3 val=69\n)\n", toString(tbl));
}


TEST(Debug, ctsm) {
  CharToStateMap m;
  m.set('a', 1);
  m.set('b', 2);
  m.set('c', 3);
  EXPECT_EQ("  1 <- a\n  2 <- b\n  3 <- c\n", toString(m));
}


TEST(Debug, dfa) {
  DfaObj dfa;
  DfaId zero = dfa.newState();
  DfaId one = dfa.newState();
  EXPECT_EQ(0, zero);
  EXPECT_EQ(1, one);
  dfa[zero].result_ = 0;
  dfa[zero].deadEnd_ = true;
  dfa[one].result_ = 1;
  dfa[one].deadEnd_ = false; // ???
  dfa[one].trans_.set('a', one);
  EXPECT_EQ("Dfa init=1 err=0\n"
            "0 DfaState -> 0\n  DeadEnd\n1 DfaState -> 1\n  1 <- a\n]\n",
            toString(dfa));
}


TEST(Debug, equiv) {
  vector<CharIdx> vec;
  vec.resize(256);
  vec['a'] = 1;
  vec['b'] = 2;
  vec['c'] = 3;
  EXPECT_EQ(
    "equiv[\n  0 <- ^@-`\n  1 <- a\n  2 <- b\n  3 <- c\n  0 <- d-$ff\n]\n",
    toString(vec));
}


TEST(Debug, header) {
  FileHeader hdr;
  hdr.magic_[0] = 'R';
  hdr.magic_[1] = 'E';
  hdr.magic_[2] = 'D';
  hdr.magic_[3] = 'B';
  hdr.majVer_ = 3;
  hdr.minVer_ = 14;
  hdr.checksum_ = 1234567890;
  hdr.format_ = 3;
  hdr.maxChar_ = 12;
  hdr.pad0_ = 0; // shouldn't matter
  hdr.stateCnt_ = 7;
  hdr.initialOff_ = 24;
  for (int ii = 0; ii < 256; ++ii)
    hdr.equivMap_[ii] = 0;
  EXPECT_EQ("REDB/3.14\ncsum=0x499602d2 fmt=3 maxChar=12\nstates=7 init=$18\n",
            toString(hdr));
}
