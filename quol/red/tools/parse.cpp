// red tool to lexically scan a regex

#include <iostream>

#include "Except.h"
#include "ReParser.h"
#include "Debug.h"

using namespace zezax::red;

int main(int argc, char **argv) {
  try {
    ReParser p;
    for (int ii = 1; ii < argc; ++ii)
      p.add(argv[ii], ii, 0);
    std::cout << toString(p.getNfa()) << std::flush;
    return 0;
  }
  catch (const RedExcept &ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
}
