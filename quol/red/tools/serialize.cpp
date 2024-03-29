// red tool to serialize a regex

#include <iostream>
#include <stdexcept>

#include "Parser.h"
#include "Powerset.h"
#include "Minimizer.h"
#include "Serializer.h"
#include "Executable.h"
#include "Matcher.h"
#include "Debug.h"

using namespace zezax::red;

using std::string;
using std::string_view;

int main(int argc, char **argv) {
  bool raw = false;
  Format fmt = fmtDirectAuto;
  try {
    string buf;
    {
      Parser p;
      int cur = 0;
      for (int ii = 1; ii < argc; ++ii) {
        string_view sv = argv[ii];
        if (sv == "-r")
          raw = true;
        else if (sv == "-1")
          fmt = fmtDirect1;
        else if (sv == "-2")
          fmt = fmtDirect2;
        else if (sv == "-4")
          fmt = fmtDirect4;
        else if (raw)
          p.add(sv, ++cur, 0);
        else
          p.addAuto(sv, ++cur, 0);
      }
      p.finish();
      {
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
        {
          Serializer ser(dfa);
          buf = ser.serializeToString(fmt);
        }
      }
    }
    std::cout << toString(buf.data(), buf.size()) << std::flush;
    Executable rex(std::move(buf));

    string line;
    while (std::getline(std::cin, line))
      std::cout << check(rex, line, styFull) << std::endl;

    return 0;
  }
  catch (const std::exception &ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
}
