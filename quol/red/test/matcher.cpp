// matching using serialized dfa: unit tests

#include <gtest/gtest.h>

#include "Parser.h"
#include "Compile.h"
#include "Exec.h"
#include "Matcher.h"

using namespace zezax::red;

using std::string;
using std::string_view;
using testing::TestWithParam;
using testing::Values;


TEST(Matcher, checkStartEnd) {
  Executable rex;
  {
    Parser p;
    p.add("[^a]*ab*c",  1, 0);
    p.add("[^d]*dummy", 2, 0);
    rex = compile(p);
  }
  Matcher mat(&rex);
  Outcome oc = mat.match("abbc", styLast);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(4, oc.end_);

  oc = mat.match("xabbc", styLast);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(1, oc.start_);
  EXPECT_EQ(5, oc.end_);

  oc = mat.match("xabbcx", styLast);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(1, oc.start_);
  EXPECT_EQ(5, oc.end_);

  oc = mat.match("xyzabbcxyz", styLast);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(3, oc.start_);
  EXPECT_EQ(7, oc.end_);
}


TEST(Matcher, checkStyles) {
  Executable rex;
  {
    Parser p;
    p.add("abc",     1, 0);
    p.add("abcd",    2, 0);
    p.add("abcdefg", 3, 0);
    rex = compile(p);
  }
  Matcher mat(&rex);
  string_view in = "abcdefg";

  EXPECT_EQ(1, mat.check(in, styFirst));
  EXPECT_EQ(2, mat.check(in, styContiguous));
  EXPECT_EQ(3, mat.check(in, styLast));
  EXPECT_EQ(3, mat.check(in, styFull));

  Outcome oc = mat.match(in, styFirst);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(3, oc.end_);

  oc = mat.match(in, styContiguous);
  EXPECT_EQ(2, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(4, oc.end_);

  oc = mat.match(in, styLast);
  EXPECT_EQ(3, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(7, oc.end_);

  oc = mat.match(in, styFull);
  EXPECT_EQ(3, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(7, oc.end_);

  in = "abcdefgh";

  EXPECT_EQ(1, mat.check(in, styFirst));
  EXPECT_EQ(2, mat.check(in, styContiguous));
  EXPECT_EQ(3, mat.check(in, styLast));
  EXPECT_EQ(0, mat.check(in, styFull));

  oc = mat.match(in, styFirst);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(3, oc.end_);

  oc = mat.match(in, styContiguous);
  EXPECT_EQ(2, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(4, oc.end_);

  oc = mat.match(in, styLast);
  EXPECT_EQ(3, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(7, oc.end_);

  oc = mat.match(in, styFull);
  EXPECT_EQ(0, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(0, oc.end_);
}


TEST(Matcher, replace) {
  Executable rex;
  {
    Parser p;
    p.add("ab*c", 1, 0);
    rex = compile(p);
  }
  Matcher mat(&rex);
  EXPECT_EQ("foobar", mat.replace("fooac", "bar", styLast));
  EXPECT_EQ("foobarz", mat.replace(string("fooacz"), "bar", styLast));
}


TEST(Matcher, replaceStyles) {
  Executable rex;
  {
    Parser p;
    p.add("abc",     1, 0);
    p.add("abcd",    2, 0);
    p.add("abcdefg", 3, 0);
    rex = compile(p);
  }
  Matcher mat(&rex);
  EXPECT_EQ("1xyzdefg2", mat.replace("1abcdefg2", "xyz", styFirst));
  EXPECT_EQ("1xyzefg2", mat.replace("1abcdefg2", "xyz", styContiguous));
  EXPECT_EQ("1xyz2", mat.replace("1abcdefg2", "xyz", styLast));
  EXPECT_EQ("1abcdefg2", mat.replace("1abcdefg2", "xyz", styFull));
  EXPECT_EQ("1xyz", mat.replace("1abcdefg", "xyz", styFull));
}


class MatcherTest : public TestWithParam<Format> {};

TEST_P(MatcherTest, check) {
  Format fmt = GetParam();
  Executable rex;
  {
    Parser p;
    p.addAuto("ab*c", 1, 0);
    p.addAuto("ca*b", 2, 0);
    rex = compile(p, fmt);
  }
  Matcher m0(&rex);
  Matcher m00(&rex);
  Matcher m1(&rex);
  Matcher m2(&rex);
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
  Executable rex;
  {
    Parser p;
    p.addAuto("ab*c", 1, 0);
    p.addAuto("ca*b", 2, 0);
    rex = compile(p, fmt);
  }
  Matcher m0(&rex);
  Matcher m00(&rex);
  Matcher m1(&rex);
  Matcher m2(&rex);
  const char *in0 = "bca";
  string in1 = "bac";
  string_view in2 = "cab";
  Outcome oc0 = m0.match(in0, styFull);
  Outcome oc00 = m00.match(in0, 3, styFull);
  Outcome oc1 = m1.match(in1, styFull);
  Outcome oc2 = m2.match(in2, styFull);
  EXPECT_EQ(0, oc0.result_);
  EXPECT_EQ(0, oc00.result_);
  EXPECT_EQ(1, oc1.result_);
  EXPECT_EQ(2, oc2.result_);
  EXPECT_EQ(0, oc0.end_);
  EXPECT_EQ(0, oc00.end_);
  EXPECT_EQ(3, oc1.end_);
  EXPECT_EQ(3, oc2.end_);
}


INSTANTIATE_TEST_SUITE_P(A, MatcherTest,
  Values(fmtDirectAuto, fmtDirect1, fmtDirect2, fmtDirect4));
