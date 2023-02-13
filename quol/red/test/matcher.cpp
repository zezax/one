// matching using serialized dfa: unit tests

#include <gtest/gtest.h>

#include "ReParser.h"
#include "Compile.h"
#include "Exec.h"
#include "Matcher.h"

using namespace zezax::red;

using std::shared_ptr;
using std::string;
using std::string_view;
using testing::TestWithParam;
using testing::Values;


TEST(Matcher, checkStartEnd) {
  shared_ptr<const Executable> rex;
  {
    ReParser p;
    p.add("[^a]*ab*c",  1, 0);
    p.add("[^d]*dummy", 2, 0);
    rex = compileShared(p);
  }
  Matcher mat(rex);
  mat.match("abbc", lenLast);
  EXPECT_EQ(1, mat.result());
  EXPECT_EQ(0, mat.start());
  EXPECT_EQ(4, mat.end());

  mat.match("xabbc", lenLast);
  EXPECT_EQ(1, mat.result());
  EXPECT_EQ(1, mat.start());
  EXPECT_EQ(5, mat.end());

  mat.match("xabbcx", lenLast);
  EXPECT_EQ(1, mat.result());
  EXPECT_EQ(1, mat.start());
  EXPECT_EQ(5, mat.end());

  mat.match("xyzabbcxyz", lenLast);
  EXPECT_EQ(1, mat.result());
  EXPECT_EQ(3, mat.start());
  EXPECT_EQ(7, mat.end());
}


TEST(Matcher, checkLengths) {
  shared_ptr<const Executable> rex;
  {
    ReParser p;
    p.add("abc",     1, 0);
    p.add("abcd",    2, 0);
    p.add("abcdefg", 3, 0);
    rex = compileShared(p);
  }
  Matcher mat(rex);
  string_view in = "abcdefg";

  EXPECT_EQ(1, mat.check(in, lenShortest));
  EXPECT_EQ(2, mat.check(in, lenContiguous));
  EXPECT_EQ(3, mat.check(in, lenLast));
  EXPECT_EQ(3, mat.check(in, lenFull));

  EXPECT_EQ(1, mat.match(in, lenShortest));
  EXPECT_EQ(1, mat.result());
  EXPECT_EQ(0, mat.start());
  EXPECT_EQ(3, mat.end());

  EXPECT_EQ(2, mat.match(in, lenContiguous));
  EXPECT_EQ(2, mat.result());
  EXPECT_EQ(0, mat.start());
  EXPECT_EQ(4, mat.end());

  EXPECT_EQ(3, mat.match(in, lenLast));
  EXPECT_EQ(3, mat.result());
  EXPECT_EQ(0, mat.start());
  EXPECT_EQ(7, mat.end());

  EXPECT_EQ(3, mat.match(in, lenFull));
  EXPECT_EQ(3, mat.result());
  EXPECT_EQ(0, mat.start());
  EXPECT_EQ(7, mat.end());

  in = "abcdefgh";

  EXPECT_EQ(1, mat.check(in, lenShortest));
  EXPECT_EQ(2, mat.check(in, lenContiguous));
  EXPECT_EQ(3, mat.check(in, lenLast));
  EXPECT_EQ(0, mat.check(in, lenFull));

  EXPECT_EQ(1, mat.match(in, lenShortest));
  EXPECT_EQ(1, mat.result());
  EXPECT_EQ(0, mat.start());
  EXPECT_EQ(3, mat.end());

  EXPECT_EQ(2, mat.match(in, lenContiguous));
  EXPECT_EQ(2, mat.result());
  EXPECT_EQ(0, mat.start());
  EXPECT_EQ(4, mat.end());

  EXPECT_EQ(3, mat.match(in, lenLast));
  EXPECT_EQ(3, mat.result());
  EXPECT_EQ(0, mat.start());
  EXPECT_EQ(7, mat.end());

  EXPECT_EQ(0, mat.match(in, lenFull));
  EXPECT_EQ(0, mat.result());
  EXPECT_EQ(0, mat.start());
  EXPECT_EQ(0, mat.end());
}


TEST(Matcher, replace) {
  shared_ptr<const Executable> rex;
  {
    ReParser p;
    p.add("ab*c", 1, 0);
    rex = compileShared(p);
  }
  Matcher mat(rex);
  EXPECT_EQ("foobar", mat.replace("fooac", "bar", lenLast));
  EXPECT_EQ("foobarz", mat.replace("fooacz", "bar", lenLast));
}


TEST(Matcher, replaceLengths) {
  shared_ptr<const Executable> rex;
  {
    ReParser p;
    p.add("abc",     1, 0);
    p.add("abcd",    2, 0);
    p.add("abcdefg", 3, 0);
    rex = compileShared(p);
  }
  Matcher mat(rex);
  EXPECT_EQ("1xyzdefg2", mat.replace("1abcdefg2", "xyz", lenShortest));
  EXPECT_EQ("1xyzefg2", mat.replace("1abcdefg2", "xyz", lenContiguous));
  EXPECT_EQ("1xyz2", mat.replace("1abcdefg2", "xyz", lenLast));
  EXPECT_EQ("1abcdefg2", mat.replace("1abcdefg2", "xyz", lenFull));
  EXPECT_EQ("1xyz", mat.replace("1abcdefg", "xyz", lenFull));
}


class MatcherTest : public TestWithParam<Format> {};

TEST_P(MatcherTest, check) {
  Format fmt = GetParam();
  shared_ptr<const Executable> rex;
  {
    ReParser p;
    p.addAuto("ab*c", 1, 0);
    p.addAuto("ca*b", 2, 0);
    rex = compileShared(p, fmt);
  }
  Matcher m0(rex);
  Matcher m00(rex);
  Matcher m1(rex);
  Matcher m2(rex);
  const char *in0 = "bca";
  string in1 = "bac";
  string_view in2 = "cab";
  EXPECT_EQ(0, m0.check(in0, lenFull));
  EXPECT_EQ(0, m00.check(in0, 3, lenFull));
  EXPECT_EQ(1, m1.check(in1, lenFull));
  EXPECT_EQ(2, m2.check(in2, lenFull));
}


TEST_P(MatcherTest, match) {
  Format fmt = GetParam();
  shared_ptr<const Executable> rex;
  {
    ReParser p;
    p.addAuto("ab*c", 1, 0);
    p.addAuto("ca*b", 2, 0);
    rex = compileShared(p, fmt);
  }
  Matcher m0(rex);
  Matcher m00(rex);
  Matcher m1(rex);
  Matcher m2(rex);
  const char *in0 = "bca";
  string in1 = "bac";
  string_view in2 = "cab";
  m0.match(in0, lenFull);
  m00.match(in0, 3, lenFull);
  m1.match(in1, lenFull);
  m2.match(in2, lenFull);
  EXPECT_EQ(0, m0.result());
  EXPECT_EQ(0, m00.result());
  EXPECT_EQ(1, m1.result());
  EXPECT_EQ(2, m2.result());
  EXPECT_EQ(0, m0.end());
  EXPECT_EQ(0, m00.end());
  EXPECT_EQ(3, m1.end());
  EXPECT_EQ(3, m2.end());
}


INSTANTIATE_TEST_SUITE_P(A, MatcherTest,
  Values(fmtOffsetAuto, fmtOffset1, fmtOffset2, fmtOffset4));
