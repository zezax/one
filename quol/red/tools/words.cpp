// red benchmark that builds a large dfa

#include <iostream>
#include <fstream>
#include <vector>

#include "Except.h"
#include "ReParser.h"
#include "Powerset.h"
#include "Minimizer.h"
#include "Serializer.h"
#include "Util.h"

#ifdef USE_JEMALLOC
# include <jemalloc.h>
#endif

using namespace zezax::red;

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
  (void) argc;
  (void) argv;

#ifdef USE_JEMALLOC
  getMallCtlStr("opt.junk");
  getMallCtlBool("opt.prof");
  getMallCtlBool("opt.prof_leak");
  getMallCtlBool("opt.prof_gdump");
  getMallCtlStr("opt.prof_prefix");
#endif

  vector<string> words;
  std::ifstream file("/usr/share/dict/words");
  string line;
  int ii = 0;
  while (std::getline(file, line))
    if ((++ii % 50) == 0)
      words.push_back(line);
  std::cout << "Total words " << words.size() << std::endl;

  try {
    ReParser p;
    Result res = 0;
    for (string &word : words)
      p.add(word, ++res, fIgnoreCase);
    NfaObj &nfa = p.getNfa();
    std::cout << "NFA orig states " << nfa.activeStates() << std::endl;
    p.finish();
    std::cout << "NFA live states " << nfa.activeStates() << std::endl;
    std::cout << "MEM " << bytesUsed() << std::endl;

    string buf;
    {
      DfaObj dfa;
      {
        PowersetConverter psc(nfa);
        dfa = psc.convert();
        p.freeAll();
      }
      std::cout << "Converted states " << dfa.numStates() << std::endl;
      std::cout << "MEM " << bytesUsed() << std::endl;

      {
        DfaMinimizer dm(dfa);
        dm.minimize();
      }
      std::cout << "Minimized states " << dfa.numStates() << std::endl;
      std::cout << "MEM " << bytesUsed() << std::endl;

      {
        Serializer ser(dfa);
        buf = ser.serialize(fmtOffsetAuto);
      }
    }
    std::cout << "Serialized bytes " << buf.size() << std::endl;
    std::cout << "MEM " << bytesUsed() << std::endl;

    return 0;
  }
  catch (const std::exception &ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
}
