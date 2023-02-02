// red benchmark that matches large text

#include <iostream>

#include "Util.h"
#include "ReParser.h"
#include "Compile.h"
#include "Exec.h"
#include "Matcher.h"

using namespace zezax::red;

using std::make_shared;
using std::shared_ptr;
using std::string;
using std::string_view;
using std::vector;

int main(int argc, char **argv) {
  (void) argc;
  (void) argv;

  try {
    string words = readFileToString("/usr/share/dict/words");

    shared_ptr<const Executable> rex;
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
      rex = compile(p);
    }
    Matcher mat(rex);

    int sum = 0;
    constexpr int iters = 1000;
    const char *beg = words.data();
    const char *end = beg + words.size();
    for (int ii = 0; ii < iters; ++ii)
      for (const char *ptr = beg; ptr < end; ) {
        mat.match<lenLast>(ptr, end - ptr);
        if (mat.result() > 0) {
          ptr += mat.end();
          sum += mat.result();
        }
        else
          ++ptr;
      }

    if (sum != (iters * 478)) { // some match multiple times
      std::cerr << "Bad sum " << sum << std::endl;
      return 1;
    }
    return 0;
  }
  catch (const std::exception &ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
}
