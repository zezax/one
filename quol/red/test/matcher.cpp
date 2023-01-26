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


TEST(Matcher, checkStartEnd) {
  string buf;
  {
    ReParser p;
    p.addRaw("[^a]*ab*c",  1, 0);
    p.addRaw("[^d]*dummy", 2, 0);
    p.finish();

    DfaObj dfa = convertNfaToDfa(p.getNfa());
    p.freeAll();
    {
      DfaMinimizer dm(dfa);
      dm.minimize();
    }
    {
      Serializer ser(dfa);
      buf = ser.serialize(fmtOffsetAuto);
    }
  }

  shared_ptr<const Executable> rex =
    make_shared<const Executable>(std::move(buf));
  Matcher mat(rex);
  mat.matchLast("abbc");
  EXPECT_EQ(1, mat.result());
  EXPECT_EQ(0, mat.start());
  EXPECT_EQ(4, mat.end());

  mat.matchLast("xabbc");
  EXPECT_EQ(1, mat.result());
  EXPECT_EQ(1, mat.start());
  EXPECT_EQ(5, mat.end());

  mat.matchLast("xabbcx");
  EXPECT_EQ(1, mat.result());
  EXPECT_EQ(1, mat.start());
  EXPECT_EQ(5, mat.end());

  mat.matchLast("xyzabbcxyz");
  EXPECT_EQ(1, mat.result());
  EXPECT_EQ(3, mat.start());
  EXPECT_EQ(7, mat.end());
}


TEST(Matcher, checkLengths) {
  string buf;
  {
    ReParser p;
    p.addRaw("abc",     1, 0);
    p.addRaw("abcd",    2, 0);
    p.addRaw("abcdefg", 3, 0);
    p.finish();

    DfaObj dfa = convertNfaToDfa(p.getNfa());
    p.freeAll();
    {
      DfaMinimizer dm(dfa);
      dm.minimize();
    }
    {
      Serializer ser(dfa);
      buf = ser.serialize(fmtOffsetAuto);
    }
  }

  shared_ptr<const Executable> rex =
    make_shared<const Executable>(std::move(buf));
  Matcher mat(rex);
  string_view in = "abcdefg";

  EXPECT_EQ(1, mat.checkShort(in));
  EXPECT_EQ(2, mat.checkContig(in));
  EXPECT_EQ(3, mat.checkLast(in));
  EXPECT_EQ(3, mat.checkWhole(in));

  EXPECT_EQ(1, mat.matchShort(in));
  EXPECT_EQ(1, mat.result());
  EXPECT_EQ(0, mat.start());
  EXPECT_EQ(3, mat.end());

  EXPECT_EQ(2, mat.matchContig(in));
  EXPECT_EQ(2, mat.result());
  EXPECT_EQ(0, mat.start());
  EXPECT_EQ(4, mat.end());

  EXPECT_EQ(3, mat.matchLast(in));
  EXPECT_EQ(3, mat.result());
  EXPECT_EQ(0, mat.start());
  EXPECT_EQ(7, mat.end());

  EXPECT_EQ(3, mat.matchWhole(in));
  EXPECT_EQ(3, mat.result());
  EXPECT_EQ(0, mat.start());
  EXPECT_EQ(7, mat.end());

  in = "abcdefgh";

  EXPECT_EQ(1, mat.checkShort(in));
  EXPECT_EQ(2, mat.checkContig(in));
  EXPECT_EQ(3, mat.checkLast(in));
  EXPECT_EQ(0, mat.checkWhole(in));

  EXPECT_EQ(1, mat.matchShort(in));
  EXPECT_EQ(1, mat.result());
  EXPECT_EQ(0, mat.start());
  EXPECT_EQ(3, mat.end());

  EXPECT_EQ(2, mat.matchContig(in));
  EXPECT_EQ(2, mat.result());
  EXPECT_EQ(0, mat.start());
  EXPECT_EQ(4, mat.end());

  EXPECT_EQ(3, mat.matchLast(in));
  EXPECT_EQ(3, mat.result());
  EXPECT_EQ(0, mat.start());
  EXPECT_EQ(7, mat.end());

  EXPECT_EQ(0, mat.matchWhole(in));
  EXPECT_EQ(0, mat.result());
  EXPECT_EQ(0, mat.start());
  EXPECT_EQ(0, mat.end());
}


TEST(Matcher, replace) {
  string buf;
  {
    ReParser p;
    p.addRaw("ab*c",  1, 0);
    p.finish();

    DfaObj dfa = convertNfaToDfa(p.getNfa());
    p.freeAll();
    {
      DfaMinimizer dm(dfa);
      dm.minimize();
    }
    {
      Serializer ser(dfa);
      buf = ser.serialize(fmtOffsetAuto);
    }
  }

  shared_ptr<const Executable> rex =
    make_shared<const Executable>(std::move(buf));
  Matcher mat(rex);
  string s = mat.replaceLast("fooac", "bar");
  EXPECT_EQ("foobar", s);
}


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
  m0.matchWhole(in0);
  m00.matchWhole(in0, 3);
  m1.matchWhole(in1);
  m2.matchWhole(in2);
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
