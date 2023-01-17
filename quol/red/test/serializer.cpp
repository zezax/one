// optimized format for dfa: unit tests

#include <gtest/gtest.h>

#include "ReParser.h"
#include "NfaToDfa.h"
#include "DfaMinimizer.h"
#include "Serializer.h"

using namespace zezax::red;

using std::string;
using std::to_string;
using testing::TestWithParam;
using testing::Values;


class SerializerTest : public TestWithParam<Format> {};

TEST_P(SerializerTest, smoke) {
  Format fmt = GetParam();
  string buf;
  {
    ReParser p;
    p.add("ab*c", 1, 0);
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
  EXPECT_EQ(nullptr, checkHeader(buf.data(), buf.size()));
}


TEST_P(SerializerTest, file) {
  Format fmt = GetParam();
  string fn = "/tmp/reda" + to_string(getpid());
  {
    ReParser p;
    p.add("ab*c", 1, 0);
    p.finish();

    DfaObj dfa = convertNfaToDfa(p.getNfa());
    p.freeAll();
    {
      DfaMinimizer dm(dfa);
      dm.minimize();
    }
    {
      Serializer ser(dfa);
      ser.serializeToFile(fmt, fn.c_str());
    }
  }
  string buf = loadFromFile(fn.c_str());
  unlink(fn.c_str());
  EXPECT_EQ(nullptr, checkHeader(buf.data(), buf.size()));
}


INSTANTIATE_TEST_SUITE_P(A, SerializerTest,
  Values(fmtOffsetAuto, fmtOffset1, fmtOffset2, fmtOffset4));
