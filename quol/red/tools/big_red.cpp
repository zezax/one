// red benchmark that builds a large dfa and then uses it for matching

#include <charconv>
#include <chrono>
#include <iostream>
#include <fstream>
#include <vector>

#include "Parser.h"
#include "Red.h"
#include "Util.h"
#include "Debug.h"

using namespace zezax::red;

namespace chrono = std::chrono;

using chrono::duration_cast;
using chrono::milliseconds;
using chrono::steady_clock;
using std::from_chars;
using std::string;
using std::string_view;
using std::vector;

int main(int argc, char **argv) {
  int iters = 1000;
  int goal = 1000;
  Flags flags = fIgnoreCase;

  for (int ii = 1; ii < argc; ++ii) {
    string_view arg = argv[ii];
    if (arg == "-cs")
      flags &= ~fIgnoreCase;
    else if ((arg == "-iter") && ((ii + 1) < argc)) {
      arg = argv[++ii];
      from_chars(arg.data(), arg.data() + arg.size(), iters);
    }
    else if ((arg == "-pat") && ((ii + 1) < argc)) {
      arg = argv[++ii];
      from_chars(arg.data(), arg.data() + arg.size(), goal);
    }
  }

  string blob = readFileToString("/usr/share/dict/words");
  vector<string> words = sampleLines(blob, goal);

  std::cout << "iterations " << iters << std::endl;
  std::cout << "patterns " << words.size() << std::endl;
  std::cout << "blobSize " << blob.size() << std::endl;

  try {
    auto t0 = steady_clock::now();

    Parser p;
    Result res = 0;
    for (const string &word : words)
      p.add(word, ++res, flags);
    Red red(p);

    auto t1 = steady_clock::now();

    size_t sum = 0;
    vector<Outcome> outs;
    for (int ii = 0; ii < iters; ++ii) {
      string_view sv = blob;
      for (; !sv.empty(); sv.remove_prefix(1))
        if (red.allMatches(sv, &outs))
          for (const Outcome &out : outs)
            sum += out.result_;
    }

    auto t2 = steady_clock::now();
    auto tTot = duration_cast<milliseconds>(t2 - t0);
    auto tComp = duration_cast<milliseconds>(t1 - t0);
    auto tMatch = duration_cast<milliseconds>(t2 - t1);
    std::cout << "totalTime " << tTot << std::endl;
    std::cout << "compileTime " << tComp << std::endl;
    std::cout << "matchTime " << tMatch << std::endl;

    std::cout << "checksum " << sum << std::endl;
    return 0;
  }
  catch (const std::exception &ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
}
