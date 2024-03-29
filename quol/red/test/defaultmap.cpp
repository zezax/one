// unit tests for default-map

#include <gtest/gtest.h>

#include "DefaultMap.h"

using namespace zezax::red;

TEST(DefaultMap, smoke) {
  DefaultMap<int, int> map;
  map.set(1, 1);
  map.set(2, 2);
  map.set(3, 0);
  map.set(100, 100);
  EXPECT_EQ(3, map.size());
  int num = 0;
  int sum = 0;
  for (int ii = 0; ii < 1000; ++ii) {
    num += 1;
    sum += map[ii];
  }
  EXPECT_EQ(1000, num);
  EXPECT_EQ(103, sum);

  DefaultMap<int, int> m2 = map;
  EXPECT_EQ(3, m2.size());
  m2 = m2;
  EXPECT_EQ(3, m2.size());
#if defined(__GNUC__) && (__GNUC__ > 10)
  m2 = std::move(m2);
  EXPECT_EQ(3, m2.size()); // seems to have special case for self-move assign
#endif /* __GNUC__ > 10 */
}
