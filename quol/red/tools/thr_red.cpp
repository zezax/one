// red threaded matching benchmark

#include <charconv>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <thread>

#include "Red.h"
#include "Util.h"
#include "Debug.h"

using namespace zezax::red;

namespace chrono = std::chrono;

using chrono::duration_cast;
using chrono::milliseconds;
using chrono::steady_clock;
using std::from_chars;
using std::make_unique;
using std::string;
using std::string_view;
using std::thread;
using std::to_string;
using std::unique_ptr;
using std::vector;


namespace {

int doWork(int iters, string_view text, Red *re) {
  int sum = 0;
  for (int ii = 0; ii < iters; ++ii) {
    string_view sv = text;
    while (!sv.empty()) {
      Outcome oc = re->matchTangent(sv);
      if (oc) {
        sv.remove_prefix(oc.end_);
        sum += 1;
      }
      else
        sv.remove_prefix(1);
    }
  }
  string s = "sum " + to_string(sum) + '\n';
  std::cout << s;
  return sum;
}

} // anonymous


int main(int argc, char **argv) {
  unsigned threadCnt = thread::hardware_concurrency();
  int iters = 100;
  Flags flags = fIgnoreCase;

  for (int ii = 1; ii < argc; ++ii) {
    string_view arg = argv[ii];
    if (arg == "-cs")
      flags &= ~fIgnoreCase;
    else if ((arg == "-thr") && ((ii + 1) < argc)) {
      arg = argv[++ii];
      from_chars(arg.data(), arg.data() + arg.size(), threadCnt);
    }
    else if ((arg == "-iter") && ((ii + 1) < argc)) {
      arg = argv[++ii];
      from_chars(arg.data(), arg.data() + arg.size(), iters);
    }
  }

  string text = readFileToString("/usr/share/dict/words");

  std::cout << "threads " << threadCnt << std::endl;
  std::cout << "iterations " << iters << std::endl;
  std::cout << "textSize " << text.size() << std::endl;

  try {
    auto t0 = steady_clock::now();

    Red re("[abc][def][ghi]", flags);

    vector<unique_ptr<thread>> workers;
    workers.reserve(threadCnt);
    for (unsigned uu = 0; uu < threadCnt; ++uu)
      workers.emplace_back(make_unique<thread>(doWork, iters, text, &re));
    for (auto &ww : workers)
      ww->join();

    auto t1 = steady_clock::now();
    auto tTot = duration_cast<milliseconds>(t1 - t0);
    std::cout << "totalTime " << tTot << std::endl;
    return 0;
  }
  catch (const std::exception &ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
}
