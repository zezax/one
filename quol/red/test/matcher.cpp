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
  mat.match("abbc", styLast);
  EXPECT_EQ(1, mat.result());
  EXPECT_EQ(0, mat.start());
  EXPECT_EQ(4, mat.end());

  mat.match("xabbc", styLast);
  EXPECT_EQ(1, mat.result());
  EXPECT_EQ(1, mat.start());
  EXPECT_EQ(5, mat.end());

  mat.match("xabbcx", styLast);
  EXPECT_EQ(1, mat.result());
  EXPECT_EQ(1, mat.start());
  EXPECT_EQ(5, mat.end());

  mat.match("xyzabbcxyz", styLast);
  EXPECT_EQ(1, mat.result());
  EXPECT_EQ(3, mat.start());
  EXPECT_EQ(7, mat.end());
}


TEST(Matcher, checkStyles) {
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

  EXPECT_EQ(1, mat.check(in, styFirst));
  EXPECT_EQ(2, mat.check(in, styContiguous));
  EXPECT_EQ(3, mat.check(in, styLast));
  EXPECT_EQ(3, mat.check(in, styFull));

  EXPECT_EQ(1, mat.match(in, styFirst));
  EXPECT_EQ(1, mat.result());
  EXPECT_EQ(0, mat.start());
  EXPECT_EQ(3, mat.end());

  EXPECT_EQ(2, mat.match(in, styContiguous));
  EXPECT_EQ(2, mat.result());
  EXPECT_EQ(0, mat.start());
  EXPECT_EQ(4, mat.end());

  EXPECT_EQ(3, mat.match(in, styLast));
  EXPECT_EQ(3, mat.result());
  EXPECT_EQ(0, mat.start());
  EXPECT_EQ(7, mat.end());

  EXPECT_EQ(3, mat.match(in, styFull));
  EXPECT_EQ(3, mat.result());
  EXPECT_EQ(0, mat.start());
  EXPECT_EQ(7, mat.end());

  in = "abcdefgh";

  EXPECT_EQ(1, mat.check(in, styFirst));
  EXPECT_EQ(2, mat.check(in, styContiguous));
  EXPECT_EQ(3, mat.check(in, styLast));
  EXPECT_EQ(0, mat.check(in, styFull));

  EXPECT_EQ(1, mat.match(in, styFirst));
  EXPECT_EQ(1, mat.result());
  EXPECT_EQ(0, mat.start());
  EXPECT_EQ(3, mat.end());

  EXPECT_EQ(2, mat.match(in, styContiguous));
  EXPECT_EQ(2, mat.result());
  EXPECT_EQ(0, mat.start());
  EXPECT_EQ(4, mat.end());

  EXPECT_EQ(3, mat.match(in, styLast));
  EXPECT_EQ(3, mat.result());
  EXPECT_EQ(0, mat.start());
  EXPECT_EQ(7, mat.end());

  EXPECT_EQ(0, mat.match(in, styFull));
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
  EXPECT_EQ("foobar", mat.replace("fooac", "bar", styLast));
  EXPECT_EQ("foobarz", mat.replace(string("fooacz"), "bar", styLast));
}


TEST(Matcher, replaceStyles) {
  shared_ptr<const Executable> rex;
  {
    ReParser p;
    p.add("abc",     1, 0);
    p.add("abcd",    2, 0);
    p.add("abcdefg", 3, 0);
    rex = compileShared(p);
  }
  Matcher mat(rex);
  EXPECT_EQ("1xyzdefg2", mat.replace("1abcdefg2", "xyz", styFirst));
  EXPECT_EQ("1xyzefg2", mat.replace("1abcdefg2", "xyz", styContiguous));
  EXPECT_EQ("1xyz2", mat.replace("1abcdefg2", "xyz", styLast));
  EXPECT_EQ("1abcdefg2", mat.replace("1abcdefg2", "xyz", styFull));
  EXPECT_EQ("1xyz", mat.replace("1abcdefg", "xyz", styFull));
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
  EXPECT_EQ(0, m0.check(in0, styFull));
  EXPECT_EQ(0, m00.check(in0, 3, styFull));
  EXPECT_EQ(1, m1.check(in1, styFull));
  EXPECT_EQ(2, m2.check(in2, styFull));
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
  m0.match(in0, styFull);
  m00.match(in0, 3, styFull);
  m1.match(in1, styFull);
  m2.match(in2, styFull);
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
  Values(fmtDirectAuto, fmtDirect1, fmtDirect2, fmtDirect4));
