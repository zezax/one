// execution of serialized dfa: unit tests

#include <gtest/gtest.h>

#include "Parser.h"
#include "Compile.h"
#include "Executable.h"
#include "Matcher.h"

using namespace zezax::red;

using std::string;
using std::string_view;
using testing::TestWithParam;
using testing::Values;

namespace {

Result execMatch(const Executable &rex, const char *str) {
  Outcome oc = match(rex, str, styFull);
  return oc.result_;
}

} // anonymous

TEST(Executable, memory) {
  string s1;
  {
    Parser p;
    p.addAuto("ab*c", 1, 0);
    s1 = compileToSerialized(p);
  }

  string s2 = s1;
  string_view sv = s2;
  size_t len = s2.size();
  char *p1 = new char[len];
  memcpy(p1, s2.data(), len);
  char *p2 = static_cast<char *>(malloc(len));
  memcpy(p2, p1, len);

  Executable e0;
  Executable e1(std::move(s1));
  Executable e2(gCopyTag, s2);
  Executable e3(gCopyTag, sv);
  Executable e4(gCopyTag, string_view(s2.data(), len));
  Executable e5(gDeleteTag, p1, len);
  Executable e6(gFreeTag, p2, len);

  EXPECT_EQ(1, execMatch(e1, "abbc"));
  EXPECT_EQ(1, execMatch(e2, "abbc"));
  EXPECT_EQ(1, execMatch(e3, "abbc"));
  EXPECT_EQ(1, execMatch(e4, "abbc"));
  EXPECT_EQ(1, execMatch(e5, "abbc"));
  EXPECT_EQ(1, execMatch(e6, "abbc"));

  e0 = std::move(e1);
  e3 = std::move(e2);
  Executable e7(std::move(e4));

  EXPECT_EQ(1, execMatch(e0, "abbc"));
  EXPECT_EQ(1, execMatch(e3, "abbc"));
  EXPECT_EQ(1, execMatch(e7, "abbc"));
};


class ExecTest : public TestWithParam<Format> {};

TEST_P(ExecTest, smoke) {
  Format fmt = GetParam();
  CompStats stats;
  Executable rex;
  {
    Parser p(nullptr, &stats);;
    p.addAuto("ab*c", 1, 0);
    p.addAuto("ca*b", 2, 0);
    rex = compile(p, fmt);
  }
  EXPECT_EQ(0, match(rex, "bca", styFull).result_);
  EXPECT_EQ(1, match(rex, "bac", styFull).result_);
  EXPECT_EQ(2, match(rex, "cab", styFull).result_);

  EXPECT_EQ(10, stats.numTokens_);
  EXPECT_EQ(2, stats.numPatterns_);
  EXPECT_LT(0, stats.origNfaStates_);
  EXPECT_LT(0, stats.usefulNfaStates_);
  EXPECT_LT(0, stats.origDfaStates_);
  EXPECT_EQ(7, stats.minimizedDfaStates_);
  EXPECT_LT(0, stats.serializedBytes_);
  EXPECT_EQ(4, stats.numDistinguishedSymbols_);
  EXPECT_LT(0, stats.transitionTableRows_);
  EXPECT_LT(0, stats.powersetMemUsed_);
}


INSTANTIATE_TEST_SUITE_P(A, ExecTest,
  Values(fmtDirectAuto, fmtDirect1, fmtDirect2, fmtDirect4));
