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
  Outcome oc = match(rex, "abbc", styLast);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(4, oc.end_);

  oc = match(rex, "xabbc", styLast);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(1, oc.start_);
  EXPECT_EQ(5, oc.end_);

  oc = match(rex, "xabbcx", styLast);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(1, oc.start_);
  EXPECT_EQ(5, oc.end_);

  oc = match(rex, "xyzabbcxyz", styLast);
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
  string_view in = "abcdefg";

  EXPECT_EQ(1, check(rex, in, styFirst));
  EXPECT_EQ(2, check(rex, in, styContiguous));
  EXPECT_EQ(3, check(rex, in, styLast));
  EXPECT_EQ(3, check(rex, in, styFull));

  Outcome oc = match(rex, in, styFirst);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(3, oc.end_);

  oc = match(rex, in, styContiguous);
  EXPECT_EQ(2, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(4, oc.end_);

  oc = match(rex, in, styLast);
  EXPECT_EQ(3, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(7, oc.end_);

  oc = match(rex, in, styFull);
  EXPECT_EQ(3, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(7, oc.end_);

  in = "abcdefgh";

  EXPECT_EQ(1, check(rex, in, styFirst));
  EXPECT_EQ(2, check(rex, in, styContiguous));
  EXPECT_EQ(3, check(rex, in, styLast));
  EXPECT_EQ(0, check(rex, in, styFull));

  oc = match(rex, in, styFirst);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(3, oc.end_);

  oc = match(rex, in, styContiguous);
  EXPECT_EQ(2, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(4, oc.end_);

  oc = match(rex, in, styLast);
  EXPECT_EQ(3, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(7, oc.end_);

  oc = match(rex, in, styFull);
  EXPECT_EQ(0, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(0, oc.end_);
}


TEST(Matcher, contiguous) {
  Executable rex;
  {
    Parser p;
    p.add("[0-9]+", 1, 0);
    rex = compile(p);
  }
  Outcome oc = match(rex, "0123456789abcdef", styFirst);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(1, oc.end_);
  oc = match(rex, "0123456789abcdef", styContiguous);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(10, oc.end_);
}


TEST(Matcher, last) {
  Executable rex;
  {
    Parser p;
    p.add("New", 1, fLooseStart);
    p.add("New York", 2, fLooseStart);
    p.add("York", 3, fLooseStart);
    rex = compile(p);
  }
  Outcome oc = match(rex, "I love New York.", styLast);
  EXPECT_EQ(2, oc.result_);
  EXPECT_EQ(7, oc.start_);
  EXPECT_EQ(15, oc.end_);
}


TEST(Matcher, replace) {
  Executable rex;
  {
    Parser p;
    p.add("ab*c", 1, 0);
    rex = compile(p);
  }
  EXPECT_EQ("foobar",  replace(rex, "fooac", "bar", styLast));
  EXPECT_EQ("foobarz", replace(rex, string("fooacz"), "bar", styLast));
  EXPECT_EQ("x,y,z", replace(rex, "xacyabbcz", ",", styContiguous));
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
  EXPECT_EQ("1xyzdefg2", replace(rex, "1abcdefg2", "xyz", styFirst));
  EXPECT_EQ("1xyzefg2", replace(rex, "1abcdefg2", "xyz", styContiguous));
  EXPECT_EQ("1xyz2", replace(rex, "1abcdefg2", "xyz", styLast));
  EXPECT_EQ("1abcdefg2", replace(rex, "1abcdefg2", "xyz", styFull));
  EXPECT_EQ("1xyz", replace(rex, "1abcdefg", "xyz", styFull));
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
  const char *in0 = "bca";
  string in1 = "bac";
  string_view in2 = "cab";
  EXPECT_EQ(0, check(rex, in0, styFull));
  EXPECT_EQ(0, check(rex, in0, 3, styFull));
  EXPECT_EQ(1, check(rex, in1, styFull));
  EXPECT_EQ(2, check(rex, in2, styFull));
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
  const char *in0 = "bca";
  string in1 = "bac";
  string_view in2 = "cab";
  Outcome oc0 = match(rex, in0, styFull);
  Outcome oc00 = match(rex, in0, 3, styFull);
  Outcome oc1 = match(rex, in1, styFull);
  Outcome oc2 = match(rex, in2, styFull);
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
