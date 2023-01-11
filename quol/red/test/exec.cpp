// execution of serialized dfa: unit tests

#include <gtest/gtest.h>

#include "ReParser.h"
#include "NfaToDfa.h"
#include "DfaMinimizer.h"
#include "Serializer.h"
#include "Exec.h"

using namespace zezax::red;

using std::string;


TEST(Exec, smoke) {
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
      buf = ser.serialize(fmtOffset4);
    }
  }

  Executable re(std::move(buf));
  EXPECT_EQ(0, re.match4("bca"));
  EXPECT_EQ(1, re.match4("bac"));
  EXPECT_EQ(2, re.match4("cab"));
}
