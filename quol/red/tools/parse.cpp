// red tool to parse a regex

#include <iostream>
#include <stdexcept>

#include "Except.h"
#include "Parser.h"
#include "Debug.h"

using namespace zezax::red;

using std::string_view;

int main(int argc, char **argv) {
  try {
    Parser p;
    int cur = 0;
    bool aut = false;
    bool glob = false;
    bool exact = false;
    Flags flags = 0;

    for (int ii = 1; ii < argc; ++ii) {
      string_view arg = argv[ii];
      if (arg == "-a")
        aut = true;
      else if (arg == "-g")
        glob = true;
      else if (arg == "-x")
        exact = true;
      else if (arg == "-i")
        flags |= fIgnoreCase;
      else if (arg == "-ls")
        flags |= fLooseStart;
      else if (arg == "-le")
        flags |= fLooseEnd;
      else if (aut)
        p.addAuto(arg, ++cur, flags);
      else if (glob)
        p.addGlob(arg, ++cur, flags);
      else if (exact)
        p.addExact(arg, ++cur, flags);
      else
        p.add(arg, ++cur, flags);
    }
    p.finish();
    std::cout << toString(p.getNfa()) << std::flush;
    return 0;
  }
  catch (const std::exception &ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
}
