// red tool to lexically scan a regex

#include <iostream>

#include "Except.h"
#include "Parser.h"
#include "Debug.h"

using namespace zezax::red;

int main(int argc, char **argv) {
  try {
    Parser p;
    for (int ii = 1; ii < argc; ++ii)
      p.addAuto(argv[ii], ii, 0);
    p.finish();
    std::cout << toString(p.getNfa()) << std::flush;
    return 0;
  }
  catch (const std::exception &ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
}
