// re2 grep-like benchmark

#include <charconv>
#include <iostream>
#include <vector>

#include "Red.h"
#include "Util.h"

using namespace zezax::red;

using std::from_chars;
using std::string;
using std::string_view;
using std::vector;

int main(int argc, char **argv) {
  int iters = 1000;
  Flags flags = fIgnoreCase;

  for (int ii = 1; ii < argc; ++ii) {
    string_view arg = argv[ii];
    if (arg == "-cs")
      flags &= ~fIgnoreCase;
    else
      from_chars(arg.data(), arg.data() + arg.size(), iters);
  }

  string blob = readFileToString("/usr/share/dict/words");

  std::cout << "Passes " << iters << std::endl;
  std::cout << "Blob size " << blob.size() << std::endl;

  try {
    // all the vowels, in order
    Red re(".*a[^\\n]*e[^\\n]*i[^\\n]*o[^\\n]*u", flags);

    size_t sum = 0;
    for (int ii = 0; ii < iters; ++ii) {
      string_view sv = blob;
      while (!sv.empty())
        if (re.prefixConsume(sv) > 0)
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
