// unit tests for utilities

#include <gtest/gtest.h>

#include <vector>

#include "Util.h"

using namespace zezax::red;

using std::vector;


TEST(Util, fromHexDigit) {
  EXPECT_EQ(0, fromHexDigit('0'));
  EXPECT_EQ(9, fromHexDigit('9'));
  EXPECT_EQ(10, fromHexDigit('a'));
  EXPECT_EQ(10, fromHexDigit('A'));
  EXPECT_EQ(15, fromHexDigit('f'));
  EXPECT_EQ(15, fromHexDigit('F'));
}


TEST(Util, safeRef) {
  vector<int> v;
  safeRef(v, 5) = 5;
  safeRef(v, 3) = 3;
  safeRef(v, 7) = 7;
  ASSERT_EQ(8, v.size());
  EXPECT_EQ(0, v[0]);
  EXPECT_EQ(0, v[1]);
  EXPECT_EQ(0, v[2]);
  EXPECT_EQ(3, v[3]);
  EXPECT_EQ(0, v[4]);
  EXPECT_EQ(5, v[5]);
  EXPECT_EQ(0, v[6]);
  EXPECT_EQ(7, v[7]);
}
