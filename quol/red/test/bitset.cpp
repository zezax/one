// unit tests for zezax::red::BitSet

#include <gtest/gtest.h>

#include "Consts.h"
#include "Debug.h"

using namespace zezax::red;

namespace {

void checkSpan(MultiChar &mc, CharIdx beg, CharIdx end) {
  mc.clearAll();
  mc.setSpan(beg, end);
  for (CharIdx ii = beg; ii <= end; ++ii)
    EXPECT_TRUE(mc.get(ii));
  EXPECT_EQ(mc.population(), end - beg + 1);
}

} // anonymous

TEST(BitSet, toString) {
  MultiChar mc;
  EXPECT_EQ("", toString(mc));
  mc.set(0);
  EXPECT_EQ("^@", toString(mc));
  mc.set(2);
  EXPECT_EQ("^@^B", toString(mc));
  mc.set(1);
  EXPECT_EQ("^@-^B", toString(mc));
  mc.setSpan('a', 'z');
  EXPECT_EQ("^@-^Ba-z", toString(mc));
  mc.set(128);
  EXPECT_EQ("^@-^Ba-z$80", toString(mc));
  mc.set(256);
  EXPECT_EQ("^@-^Ba-z$80[0]", toString(mc));
}


TEST(BitSet, setGetClearEach) {
  MultiChar mc;
  mc.clearAll();
  EXPECT_EQ(0, mc.population());
  for (CharIdx ii = 0; ii < mc.bitSize(); ++ii) {
    mc.set(ii);
    EXPECT_TRUE(mc.get(ii));
    EXPECT_EQ(1, mc.population());
    mc.clear(ii);
    EXPECT_FALSE(mc.get(ii));
    EXPECT_EQ(0, mc.population());
    EXPECT_FALSE(mc.testAndSet(ii));
    EXPECT_TRUE(mc.testAndSet(ii));
    mc.clear(ii);
  }
}


TEST(BitSet, allOps) {
  MultiChar mc;
  mc.resize(gAlphabetSize);
  mc.clearAll();
  EXPECT_EQ(0, mc.population());
  mc.setAll();
  EXPECT_EQ(mc.bitSize(), mc.population());
  mc.clearAll();
  mc.flipAll();
  EXPECT_EQ(mc.bitSize(), mc.population());
  mc.flipAll();
  EXPECT_EQ(0, mc.population());
}


TEST(BitSet, patternOps) {
  MultiChar aa;
  aa.resize(gAlphabetSize);
  for (CharIdx ii = 0; ii < aa.bitSize(); ii += 2)
    aa.set(ii);
  EXPECT_EQ(aa.population(), aa.bitSize() / 2);
  MultiChar bb;
  bb.resize(gAlphabetSize);
  for (CharIdx ii = 1; ii < bb.bitSize(); ii += 2)
    bb.set(ii);
  EXPECT_EQ(bb.population(), bb.bitSize() / 2);

  aa.unionWith(bb);
  EXPECT_EQ(aa.population(), aa.bitSize());
  aa.xorWith(bb);
  EXPECT_EQ(aa.population(), aa.bitSize() / 2);
  aa.intersectWith(bb);
  EXPECT_EQ(0, aa.population());
  aa.setAll();
  aa.intersectWith(bb);
  EXPECT_EQ(aa.population(), aa.bitSize() / 2);
}


TEST(BitSet, subsets) {
  MultiChar aa;
  MultiChar bb;
  EXPECT_FALSE(aa.hasIntersection(bb));
  EXPECT_FALSE(bb.hasIntersection(aa));
  EXPECT_TRUE(aa.contains(bb));
  EXPECT_TRUE(bb.contains(aa));
  aa.set(0);
  EXPECT_FALSE(aa.hasIntersection(bb));
  EXPECT_FALSE(bb.hasIntersection(aa));
  EXPECT_TRUE(aa.contains(bb));
  EXPECT_FALSE(bb.contains(aa));
  bb.set(63);
  EXPECT_FALSE(aa.hasIntersection(bb));
  EXPECT_FALSE(bb.hasIntersection(aa));
  EXPECT_FALSE(aa.contains(bb));
  EXPECT_FALSE(bb.contains(aa));
  bb.set(0);
  EXPECT_TRUE(aa.hasIntersection(bb));
  EXPECT_TRUE(bb.hasIntersection(aa));
  EXPECT_FALSE(aa.contains(bb));
  EXPECT_TRUE(bb.contains(aa));
  aa.subtract(bb);
  EXPECT_FALSE(aa.hasIntersection(bb));
  EXPECT_FALSE(bb.hasIntersection(aa));
  EXPECT_FALSE(aa.contains(bb));
  EXPECT_TRUE(bb.contains(aa));
  bb.subtract(bb);
  EXPECT_EQ(0, bb.population());
  aa.setSpan(0,255);
  bb.setSpan(0,127);
  EXPECT_TRUE(aa.hasIntersection(bb));
  EXPECT_TRUE(bb.hasIntersection(aa));
  EXPECT_TRUE(aa.contains(bb));
  EXPECT_FALSE(bb.contains(aa));
  EXPECT_EQ(256, aa.population());
  aa.subtract(bb);
  EXPECT_EQ(128, aa.population());
}


TEST(BitSet, spanOps) {
  MultiChar mc;
  checkSpan(mc, 0, 0);
  checkSpan(mc, 0, 255);
  checkSpan(mc, 63, 63);
  checkSpan(mc, 0, 13);
  checkSpan(mc, 13, 27);
  checkSpan(mc, 13, 63);
  checkSpan(mc, 13, 64);
  checkSpan(mc, 13, 155);
  checkSpan(mc, 128, 155);
}


TEST(BitSet, varySizes) {
  MultiChar aa;
  MultiChar bb;
  aa.setSpan(0, 36);
  bb.setSpan(0, 255);
  bb.intersectWith(aa);
  EXPECT_EQ(37, bb.population());
  EXPECT_EQ(64, bb.bitSize());

  bb.setSpan(0, 255);
  bb.resize(100);
  EXPECT_EQ(100, bb.population());

  aa.unionWith(bb);
  EXPECT_EQ(100, aa.population());

  bb.setSpan(0, 149);
  aa.xorWith(bb);
  EXPECT_EQ(50, aa.population());
}


TEST(BitSet, equal) {
  MultiChar aa;
  MultiChar bb;
  EXPECT_EQ(aa, bb);
  EXPECT_EQ(bb, aa);
  aa.set(100);
  EXPECT_NE(aa, bb);
  EXPECT_NE(bb, aa);
  bb.set(100);
  EXPECT_EQ(aa, bb);
  EXPECT_EQ(bb, aa);
  bb.set(200);
  EXPECT_NE(aa, bb);
  EXPECT_NE(bb, aa);
  bb.clear(200);
  EXPECT_EQ(aa, bb);
  EXPECT_EQ(bb, aa);
}


TEST(BitSet, less) {
  MultiChar aa;
  MultiChar bb;
  EXPECT_FALSE(aa < bb);
  EXPECT_FALSE(bb < aa);
  bb.set(100);
  EXPECT_TRUE(aa < bb);
  EXPECT_FALSE(bb < aa);
  aa.set(13);
  EXPECT_FALSE(aa < bb);
  EXPECT_TRUE(bb < aa);
}


TEST(BitSet, hash) {
  // little-endian
  MultiChar mc;
  EXPECT_EQ(0xcbf29ce484222325, mc.hash());
  mc.set(13);
  mc.set(101);
  EXPECT_EQ(0xc571e454911759a5, mc.hash());
  mc.set(1000);
  mc.clear(1000);
  EXPECT_EQ(0xc571e454911759a5, mc.hash());
}


TEST(BitSet, assign) {
  MultiChar aa;
  aa.set(100);
  MultiChar bb(aa);
  EXPECT_TRUE(bb.get(100));
  bb.set(0);
  aa = bb;
  EXPECT_TRUE(aa.get(0));

  MultiChar cc(std::move(aa));
  EXPECT_TRUE(cc.get(0));
  EXPECT_TRUE(cc.get(100));
  cc.set(63);
  bb = std::move(cc);
  EXPECT_EQ(3, bb.population());
  EXPECT_TRUE(bb.get(63));

  // moved-from should be empty, but does it matter?
  EXPECT_EQ(0, aa.population());
  EXPECT_EQ(0, cc.population());
}


TEST(BitSet, iterOps) {
  MultiChar mc;
  mc.clearAll();
  mc.set(0);
  mc.setSpan(13, 27);
  mc.set(101);
  mc.set(123);
  mc.set(236);
  MultiChar::Iter it = mc.begin();
  EXPECT_EQ(0, *it);
  EXPECT_EQ(13, *++it);
  EXPECT_EQ(14, *++it);
  EXPECT_EQ(15, *++it);
  EXPECT_EQ(16, *++it);
  EXPECT_EQ(17, *++it);
  EXPECT_EQ(18, *++it);
  EXPECT_EQ(19, *++it);
  EXPECT_EQ(20, *++it);
  EXPECT_EQ(21, *++it);
  EXPECT_EQ(22, *++it);
  EXPECT_EQ(23, *++it);
  EXPECT_EQ(24, *++it);
  EXPECT_EQ(25, *++it);
  EXPECT_EQ(26, *++it);
  EXPECT_EQ(27, *++it);
  EXPECT_EQ(101, *++it);
  EXPECT_EQ(123, *++it);
  EXPECT_EQ(236, *++it);
  mc.set(255);
  EXPECT_EQ(255, *++it);
}


TEST(BitSet, iterSkip) {
  MultiChar mc;
  mc.set(0);
  mc.set(2);
  mc.set(1);
  mc.set(128);
  MultiChar::Iter it = mc.begin();
  EXPECT_EQ(0, *it);
  EXPECT_EQ(1, *++it);
  EXPECT_EQ(2, *++it);
  EXPECT_EQ(128, *++it);
}