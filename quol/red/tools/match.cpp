// red tool to match a regex

#include <iostream>

#include "Except.h"
#include "ReParser.h"
#include "NfaToDfa.h"
#include "DfaMinimizer.h"
#include "Debug.h"

using namespace zezax::red;

using std::string;

int main(int argc, char **argv) {
  try {
    ReParser p;
    for (int ii = 1; ii < argc; ++ii)
      p.add(argv[ii], ii, 0);
    DfaObj dfa = convertNfaToDfa(p.getNfa());
    {
      DfaMinimizer dm(dfa);
      dm.minimize();
    }
    //dfa.useEquivalenceMap();
    std::cout << toString(dfa) << std::flush;

    string line;
    while (std::getline(std::cin, line))
      std::cout << "=>" << dfa.match(line) << std::endl;

    return 0;
  }
  catch (const RedExcept &ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
}
