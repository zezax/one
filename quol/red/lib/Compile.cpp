// compilation conveniences implementation

#include "Compile.h"

#include "Powerset.h"
#include "Minimizer.h"

namespace zezax::red {

using std::make_shared;
using std::shared_ptr;
using std::string;

namespace {

string doCompile(ReParser &rp, Format fmt) {
  string buf;
  rp.finish(); // idempotent
  {
    DfaObj dfa;
    {
      PowersetConverter psc(rp.getNfa());
      dfa = psc.convert();
      rp.freeAll();
    }
    {
      DfaMinimizer dm(dfa);
      dm.minimize();
    }
    {
      Serializer ser(dfa);
      buf = ser.serialize(fmt);
    }
  }

  return buf;
}

} // anonymous

Executable compile(ReParser &rp, Format fmt) {
  string buf = doCompile(rp, fmt);
  return Executable(std::move(buf));
}


shared_ptr<const Executable> compileShared(ReParser &rp, Format fmt) {
  string buf = doCompile(rp, fmt);
  return make_shared<const Executable>(std::move(buf));
}

} // namespace zezax::red
