// unit tests for utilities

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "Util.h"

using namespace zezax::red;

using std::string;
using std::to_string;
using std::vector;


TEST(Util, fromHexDigit) {
  EXPECT_EQ(0, fromHexDigit('0'));
  EXPECT_EQ(9, fromHexDigit('9'));
  EXPECT_EQ(10, fromHexDigit('a'));
  EXPECT_EQ(10, fromHexDigit('A'));
  EXPECT_EQ(15, fromHexDigit('f'));
  EXPECT_EQ(15, fromHexDigit('F'));
  EXPECT_EQ(-1, fromHexDigit('\0'));
  EXPECT_EQ(-1, fromHexDigit(' '));
  EXPECT_EQ(-1, fromHexDigit('g'));
  EXPECT_EQ(-1, fromHexDigit('~'));
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


TEST(Util, file) {
  string fn = "/tmp/reda" + to_string(getpid());
  string one;
  uint64_t acc = 0x0123456789abcdef;
  for (int ii = 0; ii < 60000; ++ii) {
    acc *= 211;
    one.append(reinterpret_cast<char *>(&acc), sizeof(acc));
  }
  writeStringToFile(one, fn.c_str());
  string two = readFileToString(fn.c_str());
  unlink(fn.c_str());
  EXPECT_EQ(one, two);
}


TEST(Util, fileFail) {
  string fn = "/proc/nonexistent8675309";
  EXPECT_THROW(readFileToString(fn.c_str()), std::system_error);
  EXPECT_THROW(writeStringToFile("foobar", fn.c_str()), std::system_error);
}


TEST(Util, bytesUsed) {
  // silly test just to exercise the code
  EXPECT_GT(bytesUsed(), 0);
}
