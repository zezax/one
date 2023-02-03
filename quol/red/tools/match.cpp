// red tool to match a regex

#include <iostream>
#include <stdexcept>

#include "ReParser.h"
#include "Powerset.h"
#include "Minimizer.h"
#include "Debug.h"

using namespace zezax::red;

using std::string;
using std::string_view;

int main(int argc, char **argv) {
  bool raw = false;
  try {
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
    std::cout << toString(dfa) << std::flush;

    string line;
    while (std::getline(std::cin, line))
      std::cout << dfa.matchWhole(line) << std::endl;

    return 0;
  }
  catch (const std::exception &ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
}
