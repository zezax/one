// matching using serialized dfa: unit tests

#include <gtest/gtest.h>

#include "ReParser.h"
#include "NfaToDfa.h"
#include "DfaMinimizer.h"
#include "Serializer.h"
#include "Exec.h"
#include "Matcher.h"

using namespace zezax::red;

using std::shared_ptr;
using std::string;
using std::string_view;
using testing::TestWithParam;
using testing::Values;


class MatcherTest : public TestWithParam<Format> {};

TEST_P(MatcherTest, check) {
  Format fmt = GetParam();
  string buf;
  {
    ReParser p;
    p.add("ab*c", 1, 0);
    p.add("ca*b", 2, 0);
    p.finish();

    DfaObj dfa = convertNfaToDfa(p.getNfa());
    p.freeAll();
    {
      DfaMinimizer dm(dfa);
      dm.minimize();
    }
    {
      Serializer ser(dfa);
      buf = ser.serialize(fmt);
    }
  }

  shared_ptr<const Executable> rex =
    make_shared<const Executable>(std::move(buf));
  Matcher m0(rex);
  Matcher m00(rex);
  Matcher m1(rex);
  Matcher m2(rex);
  const char *in0 = "bca";
  string in1 = "bac";
  string_view in2 = "cab";
  EXPECT_EQ(0, m0.checkWhole(in0));
  EXPECT_EQ(0, m00.checkWhole(in0, 3));
  EXPECT_EQ(1, m1.checkWhole(in1));
  EXPECT_EQ(2, m2.checkWhole(in2));
}


TEST_P(MatcherTest, match) {
  Format fmt = GetParam();
  string buf;
  {
    ReParser p;
    p.add("ab*c", 1, 0);
    p.add("ca*b", 2, 0);
    p.finish();

    DfaObj dfa = convertNfaToDfa(p.getNfa());
    p.freeAll();
    {
      DfaMinimizer dm(dfa);
      dm.minimize();
    }
    {
      Serializer ser(dfa);
      buf = ser.serialize(fmt);
    }
  }

  shared_ptr<const Executable> rex =
    make_shared<const Executable>(std::move(buf));
  Matcher m0(rex);
  Matcher m00(rex);
  Matcher m1(rex);
  Matcher m2(rex);
  const char *in0 = "bca";
  string in1 = "bac";
  string_view in2 = "cab";
  m0.matchLong(in0);
  m00.matchLong(in0, 3);
  m1.matchLong(in1);
  m2.matchLong(in2);
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
