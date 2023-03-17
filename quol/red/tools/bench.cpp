// red benchmark that matches large text

#include <charconv>
#include <iostream>

#include "Util.h"
#include "Parser.h"
#include "Compile.h"
#include "Exec.h"
#include "Matcher.h"

using namespace zezax::red;

using std::from_chars;
using std::string;
using std::string_view;
using std::vector;

int main(int argc, char **argv) {
  int iters = 1000;

  for (int ii = 1; ii < argc; ++ii) {
    string_view arg = argv[ii];
    from_chars(arg.data(), arg.data() + arg.size(), iters);
  }

  try {
    string words = readFileToString("/usr/share/dict/words");

    Executable rex;
    {
      Parser p;
      p.add("anticritique",   1, fIgnoreCase);
      p.add("beerbibber",     2, fIgnoreCase);
      p.add("capriped",       3, fIgnoreCase);
      p.add("commorth",       4, fIgnoreCase);
      p.add("degradedness",   5, fIgnoreCase);
      p.add("elderberry",     6, fIgnoreCase);
      p.add("firelit",        7, fIgnoreCase);
      p.add("groove",         8, fIgnoreCase);
      p.add("hypotoxicity",   9, fIgnoreCase);
      p.add("jongleur",      10, fIgnoreCase);
      p.add("malapropism",   11, fIgnoreCase);
      p.add("multifoiled",   12, fIgnoreCase);
      p.add("ombrophoby",    13, fIgnoreCase);
      p.add("pauperdom",     14, fIgnoreCase);
      p.add("polysyllabic",  15, fIgnoreCase);
      p.add("pygopagus",     16, fIgnoreCase);
      p.add("romerillo",     17, fIgnoreCase);
      p.add("shrimplike",    18, fIgnoreCase);
      p.add("stridence",     19, fIgnoreCase);
      p.add("tetrapoda",     20, fIgnoreCase);
      p.add("unadventuring", 21, fIgnoreCase);
      p.add("unpretending",  22, fIgnoreCase);
      p.add("waxily",        23, fIgnoreCase);
      rex = compile(p);
    }

    int sum = 0;
    for (int ii = 0; ii < iters; ++ii) {
      string_view sv = words;
      while (!sv.empty()) {
        Outcome oc = matchNaive<styLast>(rex, sv);
        if (oc) {
          sv.remove_prefix(oc.end_);
          sum += oc.result_;
        }
        else
          sv.remove_prefix(1);
      }
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
