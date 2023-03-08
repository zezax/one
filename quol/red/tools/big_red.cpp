// red benchmark that builds a large dfa and then uses it for matching

#include <charconv>
#include <iostream>
#include <fstream>
#include <vector>

#include "Parser.h"
#include "Red.h"
#include "Util.h"

using namespace zezax::red;

using std::from_chars;
using std::string;
using std::string_view;
using std::vector;

int main(int argc, char **argv) {
  constexpr int iters = 1000;
  int goal = 1000;
  Flags flags = fIgnoreCase;

  for (int ii = 1; ii < argc; ++ii) {
    string_view arg = argv[ii];
    if (arg == "-cs")
      flags &= ~fIgnoreCase;
    else
      from_chars(arg.data(), arg.data() + arg.size(), goal);
  }

  string blob = readFileToString("/usr/share/dict/words");
  vector<string> words = sampleLines(blob, goal);

  std::cout << "Passes " << iters << std::endl;
  std::cout << "Total words " << words.size() << std::endl;
  std::cout << "Blob size " << blob.size() << std::endl;

  try {
    Parser p;
    Result res = 0;
    for (const string &word : words)
      p.add(word, ++res, flags);
    Red red(p);

    size_t sum = 0;
    vector<Outcome> outs;
    const char *beg = blob.data();
    const char *end = beg + blob.size();
    for (int ii = 0; ii < iters; ++ii)
      for (const char *ptr = beg; ptr < end; ++ptr) {
        string_view sv(ptr, end);
        if (red.allMatches(sv, &outs))
          for (const Outcome &out : outs) {
            sum += out.result_;
          }
      }

    std::cout << "Checksum " << sum << std::endl;
    return 0;
  }
  catch (const std::exception &ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
}
