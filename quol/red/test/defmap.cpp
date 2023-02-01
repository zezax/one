// unit tests for default-map

#include <gtest/gtest.h>

#include "DefMap.h"

using namespace zezax::red;

TEST(DefMap, smoke) {
  DefaultMap<int, int> map;
  map.set(1, 1);
  map.set(2, 2);
  map.set(3, 0);
  map.set(100, 100);
  EXPECT_EQ(3, map.getMap().size());
  int num = 0;
  int sum = 0;
  for (int ii = 0; ii < 1000; ++ii) {
    num += 1;
    sum += map[ii];
  }
  EXPECT_EQ(1000, num);
  EXPECT_EQ(103, sum);
}
