// optimized format for dfa: unit tests

#include <gtest/gtest.h>

#include <array>

#include "Parser.h"
#include "Powerset.h"
#include "Minimizer.h"
#include "Serializer.h"

using namespace zezax::red;

using std::string;
using std::to_string;
using testing::TestWithParam;
using testing::Values;


TEST(Serializer, header) {
  std::array<char, 1024> buf;
  buf.fill(0);
  EXPECT_NE(nullptr, checkHeader(buf.data(), buf.size()));
}


class SerializerTest : public TestWithParam<Format> {};

TEST_P(SerializerTest, smoke) {
  Format fmt = GetParam();
  string buf;
  {
    Parser p;
    p.addAuto("ab*c", 1, 0);
    p.finish();
    {
      DfaObj dfa;
      {
        PowersetConverter psc(p.getNfa());
        dfa = psc.convert();
        p.freeAll();
      }
      {
        DfaMinimizer dm(dfa);
        dm.minimize();
      }
      {
        Serializer ser(dfa);
        buf = ser.serializeToString(fmt);
      }
    }
  }
  EXPECT_EQ(nullptr, checkHeader(buf.data(), buf.size()));
}


TEST_P(SerializerTest, file) {
  Format fmt = GetParam();
  string fn = "/tmp/reda" + to_string(getpid());
  {
    Parser p;
    p.addAuto("ab*c", 1, 0);
    p.finish();
    {
      DfaObj dfa;
      {
        PowersetConverter psc(p.getNfa());
        dfa = psc.convert();
        p.freeAll();
      }
      {
        DfaMinimizer dm(dfa);
        dm.minimize();
      }
      {
        Serializer ser(dfa);
        ser.serializeToFile(fmt, fn.c_str());
      }
    }
  }
  string buf = loadFromFile(fn.c_str());
  unlink(fn.c_str());
  EXPECT_EQ(nullptr, checkHeader(buf.data(), buf.size()));
}


INSTANTIATE_TEST_SUITE_P(A, SerializerTest,
  Values(fmtDirectAuto, fmtDirect1, fmtDirect2, fmtDirect4));
