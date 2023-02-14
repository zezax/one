// compilation conveniences implementation

#include "Compile.h"

#include "Powerset.h"
#include "Minimizer.h"

namespace zezax::red {

using std::make_shared;
using std::shared_ptr;
using std::string;


Executable compile(ReParser &rp, Format fmt, CompStats *stats) {
  string buf = compileToSerialized(rp, fmt, stats);
  return Executable(std::move(buf));
}


shared_ptr<const Executable> compileShared(ReParser  &rp,
                                           Format     fmt,
                                           CompStats *stats) {
  string buf = compileToSerialized(rp, fmt, stats);
  return make_shared<const Executable>(std::move(buf));
}


string compileToSerialized(ReParser &rp, Format fmt, CompStats *stats) {
  string buf;
  rp.finish(); // idempotent, except for stats
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
      buf = ser.serialize(fmt);
    }
  }

  return buf;
}

} // namespace zezax::red
