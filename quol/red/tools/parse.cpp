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
    Flags flags = 0;
    Language lang = langRegexRaw;

    for (int ii = 1; ii < argc; ++ii) {
      string_view arg = argv[ii];
      if (arg == "-a")
        lang = langRegexAuto;
      else if (arg == "-g")
        lang = langGlob;
      else if (arg == "-x")
        lang = langExact;
      else if (arg == "-i")
        flags |= fIgnoreCase;
      else if (arg == "-ls")
        flags |= fLooseStart;
      else if (arg == "-le")
        flags |= fLooseEnd;
      else
        p.addAs(lang, arg, ++cur, flags);
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
