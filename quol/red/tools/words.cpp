// red benchmark that builds a large dfa

#include <charconv>
#include <iostream>
#include <fstream>
#include <vector>

#include "Except.h"
#include "Parser.h"
#include "Compile.h"
#include "Exec.h"
#include "Util.h"
#include "Debug.h"

#ifdef USE_JEMALLOC
# include <jemalloc.h>
#endif

using namespace zezax::red;

using std::from_chars;
using std::string;
using std::string_view;
using std::vector;

#ifdef USE_JEMALLOC
const char *malloc_conf = "prof:true,lg_prof_sample:20,lg_prof_interval:25,prof_prefix:/tmp/jeprof";

namespace {

void getMallCtlStr(const char *name) {
  const char *str;
  size_t len = sizeof(str);
  mallctl(name, &str, &len, nullptr, 0);
  std::cout << "jemalloc " << name << '=' << str << std::endl;
}


void getMallCtlBool(const char *name) {
  bool val;
  size_t len = sizeof(val);
  mallctl(name, &val, &len, nullptr, 0);
  std::cout << "jemalloc " << name << '=' << val << std::endl;
}

} // anonymous
#endif /* USE_JEMALLOC */

int main(int argc, char **argv) {

#ifdef USE_JEMALLOC
  getMallCtlStr("opt.junk");
  getMallCtlBool("opt.prof");
  getMallCtlBool("opt.prof_leak");
  getMallCtlBool("opt.prof_gdump");
  getMallCtlStr("opt.prof_prefix");
#endif

  int goal = 0;
  Flags flags = fIgnoreCase;

  for (int ii = 1; ii < argc; ++ii) {
    string_view arg = argv[ii];
    if (arg == "-ls")
      flags |= fLooseStart;
    else if (arg == "-le")
      flags |= fLooseEnd;
    else if (arg == "-cs")
      flags &= ~fIgnoreCase;
    else
      from_chars(arg.data(), arg.data() + arg.size(), goal);
  }

  constexpr int totWords = 234937; // !!! from my machine
  if (!goal)
    goal = 9397; // for continuity with past tests
  int ratio = totWords / goal;
  std::cout << "Goal words " << goal << " ratio 1:" << ratio << std::endl;

  vector<string> words;
  {
    std::ifstream file("/usr/share/dict/words");
    string line;
    int lines = 0;
    int hits = 0;
    while (std::getline(file, line)) {
      if ((++lines % ratio) == 0) {
        words.push_back(line);
        if (++hits >= goal)
          break;
      }
    }
  }
  std::cout << "Total words " << words.size() << std::endl;

  try {
    CompStats stats;
    Executable rex;
    {
      Parser p(&stats);
      Result res = 0;
      for (string &word : words)
        p.add(word, ++res, flags);
      rex = compile(p, fmtDirectAuto, &stats);
    }

    std::cout << toString(&stats) << std::endl;

    return 0;
  }
  catch (const std::exception &ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
}
