// red tool to lexically scan a regex

#include <iostream>
#include <stdexcept>

#include "Except.h"
#include "Scanner.h"
#include "Debug.h"

using namespace zezax::red;

int main(int argc, char **argv) {
  try {
    Scanner sc;
    for (int ii = 1; ii < argc; ++ii) {
      sc.init(argv[ii]);
      for (;;) {
        Token tok = sc.scanNext();
        std::cout << toString(tok) << std::endl;
        if (tok.type_ <= tEnd)
          break;
      }
    }
    return 0;
  }
  catch (const std::exception &ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
}
