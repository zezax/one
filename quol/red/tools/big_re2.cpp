// re2 benchmark that builds a large set and then uses it for matching

#include <charconv>
#include <chrono>
#include <iostream>
#include <vector>

#include <re2/set.h>

#include "Util.h"
#include "Debug.h"

using namespace re2;
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
  RE2::Options opt(RE2::Latin1);
  opt.set_max_mem(8L * 1024 * 1024 * 1024);
  opt.set_dot_nl(true);
  opt.set_case_sensitive(false);
  RE2::Anchor anc = RE2::ANCHOR_START;

  for (int ii = 1; ii < argc; ++ii) {
    string_view arg = argv[ii];
    if (arg == "-cs")
      opt.set_case_sensitive(true);
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

    RE2::Set rset(opt, anc);
    for (const string &word : words) {
      int res = rset.Add(word, nullptr); // returns one less than traditional
      if (res < 0)
        throw std::runtime_error("failed to parse regex");
    }
    if (!rset.Compile())
      throw std::runtime_error("failed to compile regex set");

    auto t1 = steady_clock::now();

    size_t sum = 0;
    vector<int> indices;
    for (int ii = 0; ii < iters; ++ii) {
      StringPiece sp = blob;
      for (; !sp.empty(); sp.remove_prefix(1))
        if (rset.Match(sp, &indices))
          for (int idx : indices)
            sum += idx + 1; // add one to match traditional numbering
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
