// red tool to serialize a regex

#include <iostream>
#include <stdexcept>

#include "ReParser.h"
#include "NfaToDfa.h"
#include "DfaMinimizer.h"
#include "Serializer.h"
#include "Exec.h"
#include "Matcher.h"
#include "Debug.h"

using namespace zezax::red;

using std::shared_ptr;
using std::string;
using std::string_view;

int main(int argc, char **argv) {
  bool raw = false;
  Format fmt = fmtOffset4;
  try {
    string buf;
    {
      ReParser p;
      int cur = 0;
      for (int ii = 1; ii < argc; ++ii) {
        string_view sv = argv[ii];
        if (sv == "-r")
          raw = true;
        else if (raw)
          p.addRaw(sv, ++cur, 0);
        else
          p.add(sv, ++cur, 0);
      }
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
    std::cout << toString(buf.data(), buf.size()) << std::flush;
    shared_ptr<const Executable> rex =
      make_shared<const Executable>(std::move(buf));
    Matcher mat(rex);

    string line;
    while (std::getline(std::cin, line))
      std::cout << mat.checkWhole(line) << std::endl;

    return 0;
  }
  catch (const std::exception &ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
}
