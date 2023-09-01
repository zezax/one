// red tool to parse a regex

#include <iostream>
#include <stdexcept>

#include "Except.h"
#include "Parser.h"
#include "Debug.h"

using namespace zezax::red;

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
    std::cout << toString(p.getNfa()) << std::flush;
    return 0;
  }
  catch (const std::exception &ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
}
