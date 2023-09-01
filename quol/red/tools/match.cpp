// red tool to match a regex

#include <iostream>
#include <stdexcept>

#include "Parser.h"
#include "Powerset.h"
#include "Minimizer.h"
#include "Debug.h"

using namespace zezax::red;

using std::string;
using std::string_view;

int main(int argc, char **argv) {
  bool raw = false;
  bool glob = false;
  try {
    Parser p;
    int cur = 0;
    for (int ii = 1; ii < argc; ++ii) {
      string_view arg = argv[ii];
      if (arg == "-r")
        raw = true;
      else if (arg == "-g")
        glob = true;
      else if (raw)
        p.add(arg, ++cur, 0);
      else if (glob)
        p.addGlob(arg, ++cur, 0);
      else
        p.addAuto(arg, ++cur, 0);
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
      std::cout << dfa.matchFull(line) << std::endl;

    return 0;
  }
  catch (const std::exception &ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
}
