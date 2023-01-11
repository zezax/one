// red benchmark that matches large text

#include <iostream>

#include "Util.h"
#include "ReParser.h"
#include "NfaToDfa.h"
#include "DfaMinimizer.h"
#include "Serializer.h"
#include "Exec.h"

using namespace zezax::red;

using std::string;
using std::string_view;
using std::vector;

int main(int argc, char **argv) {
  (void) argc;
  (void) argv;

  try {
    string words = readFileToString("/usr/share/dict/words");

    string buf;

    {
      ReParser p;
      p.addRaw("anticritique",   1, fIgnoreCase);
      p.addRaw("beerbibber",     2, fIgnoreCase);
      p.addRaw("capriped",       3, fIgnoreCase);
      p.addRaw("commorth",       4, fIgnoreCase);
      p.addRaw("degradedness",   5, fIgnoreCase);
      p.addRaw("elderberry",     6, fIgnoreCase);
      p.addRaw("firelit",        7, fIgnoreCase);
      p.addRaw("groove",         8, fIgnoreCase);
      p.addRaw("hypotoxicity",   9, fIgnoreCase);
      p.addRaw("jongleur",      10, fIgnoreCase);
      p.addRaw("malapropism",   11, fIgnoreCase);
      p.addRaw("multifoiled",   12, fIgnoreCase);
      p.addRaw("ombrophoby",    13, fIgnoreCase);
      p.addRaw("pauperdom",     14, fIgnoreCase);
      p.addRaw("polysyllabic",  15, fIgnoreCase);
      p.addRaw("pygopagus",     16, fIgnoreCase);
      p.addRaw("romerillo",     17, fIgnoreCase);
      p.addRaw("shrimplike",    18, fIgnoreCase);
      p.addRaw("stridence",     19, fIgnoreCase);
      p.addRaw("tetrapoda",     20, fIgnoreCase);
      p.addRaw("unadventuring", 21, fIgnoreCase);
      p.addRaw("unpretending",  22, fIgnoreCase);
      p.addRaw("waxily",        23, fIgnoreCase);
      p.finish();

      DfaObj dfa = convertNfaToDfa(p.getNfa());
      p.freeAll();
      {
        DfaMinimizer dm(dfa);
        dm.minimize();
      }
      {
        Serializer ser(dfa);
        buf = ser.serialize(fmtOffset4);
      }
    }

    Executable rex(std::move(buf));

    int rv;
    const char *beg = words.data();
    const char *end = beg + words.size();
    for (int ii = 0; ii < 1000; ++ii) {
      for (const char *ptr = beg; ptr < end; ++ptr)
        rv += rex.match4(string_view(ptr, end));
    }

    return rv;
  }
  catch (const std::exception &ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
}
