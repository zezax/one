// unit tests for utilities

#include <gtest/gtest.h>

#include "Debug.h"

using namespace zezax::red;


TEST(Debug, toHexDigit) {
  EXPECT_EQ('0', toHexDigit(0));
  EXPECT_EQ('9', toHexDigit(9));
  EXPECT_EQ('a', toHexDigit(10));
  EXPECT_EQ('f', toHexDigit(15));
}


TEST(Debug, toHexString) {
  EXPECT_EQ("0", toHexString(0));
  EXPECT_EQ("9", toHexString(9));
  EXPECT_EQ("a", toHexString(10));
  EXPECT_EQ("f", toHexString(15));
  EXPECT_EQ("ff", toHexString(255));
  EXPECT_EQ("ffff", toHexString(65535));
}
