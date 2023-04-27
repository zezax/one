// re2 grep-like benchmark

#include <charconv>
#include <iostream>
#include <vector>

#include <re2/set.h>

#include "Util.h"

using namespace re2;
using namespace zezax::red;

using std::from_chars;
using std::string;
using std::string_view;
using std::vector;

int main(int argc, char **argv) {
  int iters = 1000;
  RE2::Options opt(RE2::Latin1);
  opt.set_max_mem(8L * 1024 * 1024 * 1024);
  opt.set_dot_nl(true);
  opt.set_case_sensitive(false);

  for (int ii = 1; ii < argc; ++ii) {
    string_view arg = argv[ii];
    if (arg == "-cs")
      opt.set_case_sensitive(true);
    else
      from_chars(arg.data(), arg.data() + arg.size(), iters);
  }

  string blob = readFileToString("/usr/share/dict/words");

  std::cout << "Passes " << iters << std::endl;
  std::cout << "Blob size " << blob.size() << std::endl;

  try {
    // all the vowels, in order
    // use non-greedy initial match to make it work like a dfa
    RE2 re(".*?a[^\\n]*e[^\\n]*i[^\\n]*o[^\\n]*u", opt);
    if (!re.ok())
        throw std::runtime_error("failed to create regex");

    size_t sum = 0;
    for (int ii = 0; ii < iters; ++ii) {
      StringPiece sp = blob;
      while (!sp.empty())
        if (RE2::Consume(&sp, re))
          ++sum;
        else
          break;
    }

    std::cout << "Total " << sum << std::endl;
    return 0;
  }
  catch (const std::exception &ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
}
