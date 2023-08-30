/* Compile.cpp - compilation conveniences implementation

   See general description in Compile.h
*/

#include "Compile.h"

#include "Powerset.h"
#include "Minimizer.h"

namespace zezax::red {

using std::string;


Executable compile(Parser &rp, Format fmt) {
  string buf = compileToSerialized(rp, fmt);
  return Executable(std::move(buf));
}


string compileToSerialized(Parser &rp, Format fmt) {
  string buf;
  Budget *budget   = rp.getBudget();
  CompStats *stats = rp.getStats();
  rp.finish(); // idempotent
  {
    DfaObj dfa(budget);
    {
      PowersetConverter psc(rp.getNfa(), budget, stats);
      dfa = psc.convert();
      rp.freeAll();
    }
    {
      DfaMinimizer dm(dfa, stats);
      dm.minimize();
    }
    {
      Serializer ser(dfa, stats);
      buf = ser.serializeToString(fmt);
    }
  }

  return buf;
}

} // namespace zezax::red
