// execution of serialized dfa: unit tests

#include <gtest/gtest.h>

#include "ReParser.h"
#include "Compile.h"
#include "Exec.h"
#include "Matcher.h"

using namespace zezax::red;

using std::string;
using testing::TestWithParam;
using testing::Values;


class Exec : public TestWithParam<Format> {};

TEST_P(Exec, smoke) {
  Format fmt = GetParam();
  Executable rex;
  {
    ReParser p;
    p.addAuto("ab*c", 1, 0);
    p.addAuto("ca*b", 2, 0);
    rex = compile(p, fmt);
  }
  Matcher mat(&rex);
  EXPECT_EQ(0, mat.match("bca", lenFull));
  EXPECT_EQ(1, mat.match("bac", lenFull));
  EXPECT_EQ(2, mat.match("cab", lenFull));
}


INSTANTIATE_TEST_SUITE_P(A, Exec,
  Values(fmtDirectAuto, fmtDirect1, fmtDirect2, fmtDirect4));
