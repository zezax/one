// compilation conveniences implementation

#include "Compile.h"

#include "Powerset.h"
#include "Minimizer.h"

namespace zezax::red {

using std::string;


Executable compile(Parser &rp, Format fmt, CompStats *stats) {
  string buf = compileToSerialized(rp, fmt, stats);
  return Executable(std::move(buf));
}


string compileToSerialized(Parser &rp, Format fmt, CompStats *stats) {
  string buf;
  rp.finish(); // idempotent
  {
    DfaObj dfa;
    {
      PowersetConverter psc(rp.getNfa(), stats);
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
