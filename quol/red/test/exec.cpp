// execution of serialized dfa: unit tests

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
using testing::TestWithParam;
using testing::Values;


class Exec : public TestWithParam<Format> {};

TEST_P(Exec, smoke) {
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
  Matcher mat(rex);
  EXPECT_EQ(0, mat.matchLong("bca"));
  EXPECT_EQ(1, mat.matchLong("bac"));
  EXPECT_EQ(2, mat.matchLong("cab"));
}


INSTANTIATE_TEST_SUITE_P(A, Exec,
  Values(fmtOffsetAuto, fmtOffset1, fmtOffset2, fmtOffset4));
