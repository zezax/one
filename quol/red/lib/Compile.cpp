// compilation conveniences implementation

#include "Compile.h"

#include "NfaToDfa.h"
#include "DfaMinimizer.h"

namespace zezax::red {

using std::make_shared;
using std::shared_ptr;
using std::string;

shared_ptr<const Executable> compile(ReParser &rp, Format fmt) {
  string buf;
  rp.finish(); // idempotent
  {
    DfaObj dfa = convertNfaToDfa(rp.getNfa());
    rp.freeAll();
    {
      DfaMinimizer dm(dfa);
      dm.minimize();
    }
    {
      Serializer ser(dfa);
      buf = ser.serialize(fmt);
    }
  }

  return make_shared<const Executable>(std::move(buf));
}

} // namespace zezax::red
