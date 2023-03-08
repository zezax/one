// re2 benchmark that builds a large set and then uses it for matching

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
  constexpr int iters = 1000;
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
    else
      from_chars(arg.data(), arg.data() + arg.size(), goal);
  }

  string blob = readFileToString("/usr/share/dict/words");
  vector<string> words = sampleLines(blob, goal);

  std::cout << "Passes " << iters << std::endl;
  std::cout << "Total words " << words.size() << std::endl;
  std::cout << "Blob size " << blob.size() << std::endl;

  try {
    RE2::Set rset(opt, anc);
    for (const string &word : words) {
      int res = rset.Add(word, nullptr); // returns one less than traditional
      if (res < 0)
        throw std::runtime_error("failed to parse regex");
    }
    if (!rset.Compile())
      throw std::runtime_error("failed to compile regex set");

    size_t sum = 0;
    vector<int> indices;
    const char *beg = blob.data();
    const char *end = beg + blob.size();
    for (int ii = 0; ii < iters; ++ii)
      for (const char *ptr = beg; ptr < end; ++ptr) {
        StringPiece sp(ptr, end - ptr);
        if (rset.Match(sp, &indices))
          for (int idx : indices) {
            sum += idx + 1; // add one to match traditional numbering
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
