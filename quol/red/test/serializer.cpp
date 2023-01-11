// optimized format for dfa: unit tests

#include <gtest/gtest.h>

#include "ReParser.h"
#include "NfaToDfa.h"
#include "DfaMinimizer.h"
#include "Serializer.h"

using namespace zezax::red;

using std::string;
using std::to_string;


TEST(Serializer, smoke) {
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
      buf = ser.serialize(fmtOffset4);
    }
  }
  EXPECT_EQ(nullptr, checkHeader(buf.data(), buf.size()));
}


TEST(Serializer, file) {
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
      ser.serializeToFile(fmtOffset4, fn.c_str());
    }
  }
  string buf = loadFromFile(fn.c_str());
  unlink(fn.c_str());
  EXPECT_EQ(nullptr, checkHeader(buf.data(), buf.size()));
}
