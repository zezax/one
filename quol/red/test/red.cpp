// unit tests for RED API

#include <gtest/gtest.h>

#include "Red.h"
#include "Parser.h"

using namespace zezax::red;


TEST(Red, smoke) {
  EXPECT_TRUE(Red::fullMatch("hello", "h.*o"));
  EXPECT_FALSE(Red::fullMatch("hello", "e"));
  EXPECT_TRUE(Red::prefixMatch("hello", "h.*l"));
  EXPECT_FALSE(Red::prefixMatch("hello", "e"));
  EXPECT_TRUE(Red::partialMatch("hello", "e"));
  EXPECT_FALSE(Red::partialMatch("hello", "z"));

  Parser p;
  p.add("alex", 1, 0);
  p.add("meyer", 2, 0);
  EXPECT_TRUE(Red::partialMatch("alexmeyer.com", p));
}
