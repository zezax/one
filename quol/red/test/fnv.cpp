// unit tests for fnv hash

#include <cstdint>

#include <gtest/gtest.h>

#include "Fnv.h"

using namespace zezax::red;

TEST(Fnv, smoke) {
  char buf[] = "chongo was here!\n"; // 18 bytes including NUL
  EXPECT_EQ(0xcbf29ce484222325, fnv1a<uint64_t>(buf, 0));
  EXPECT_EQ(0xaf63de4c8601eff2, fnv1a<uint64_t>(buf, 1));
  EXPECT_EQ(0x08a25607b54a22ae, fnv1a<uint64_t>(buf, 2));
  EXPECT_EQ(0x46810940eff5f915, fnv1a<uint64_t>(buf, sizeof(buf) - 1));
  uint64_t h0 = fnv1a<uint64_t>(buf, 6);
  EXPECT_EQ(0xe150688c8217b8fd, h0);
  uint64_t h1 = fnv1aInc<uint64_t>(h0, buf + 6, sizeof(buf) - 7);
  EXPECT_EQ(0x46810940eff5f915, h1);
}
