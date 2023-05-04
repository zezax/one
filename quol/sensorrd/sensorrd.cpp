// sensorrd.cpp - main file

#include <chrono>
#include <iostream>
#include <thread>

#include "wrapper.h"

namespace chrono = std::chrono;
namespace this_thread = std::this_thread;

using namespace zezax::sensorrd;

using namespace std::chrono_literals;

using chrono::system_clock;
using std::string;
using std::string_view;

namespace {

bool endswith(string_view str, const char *tail) {
  string_view tv = tail;
  if (str.size() < tv.size())
    return false;
  string_view suf(str.data() + (str.size() - tv.size()), tv.size());
  return (suf == tv);
}

} // anonymous

int main(int argc, char **argv) {
  (void) argc; // FIXME rrdpath
  (void) argv;

  chrono::nanoseconds period = 60s;
  auto when = system_clock::now();

  ContextPtrT ctx = std::make_shared<ContextT>();

  for (;;) { // FIXME quitflag

    for (ChipIterT citer(ctx); citer; ++citer) {
      std::cout << ChipT::toString(citer.getChip()) << std::endl;
      for (FeatureIterT fiter(ctx, citer.getChip()); fiter; ++fiter) {
        std::cout << "  " << fiter.getLabel() << std::endl;
        string key;
        double val = 0.0;
        bool alarm = false;
        for (SubIterT siter(ctx, citer.getChip(), fiter.getFeature());
             siter; ++siter) {
          string_view sv = siter.getName();
          if (endswith(sv, "_input")) {
            key.assign(sv, 0, sv.size() - 6);
            val = siter.getVal();
          }
          if (endswith(sv, "_alarm") && (siter.getVal() > 0.0))
            alarm = true;
        }
        if (!key.empty()) {
          if (alarm)
            std::cout << "    " << key << '=' << val << " [ALARM]" << std::endl;
          else
            std::cout << "    " << key << '=' << val << std::endl;
        }
      }
    }

    for (AllIterT all(ctx); all; ++all) {
      std::cout << ChipT::toString(all.getChip()) << ':'
                << all.getLabel() << ':'
                << all.getName() << '='
                << all.getVal() << std::endl;
    }

    when += period;
    this_thread::sleep_until(when);
  }
  return 0;
}
