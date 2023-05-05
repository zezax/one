// sensorrd.cpp - main file

#include <chrono>
#include <iostream>
#include <set>
#include <thread>
#include <vector>

#include "wrapper.h"

namespace chrono = std::chrono;
namespace this_thread = std::this_thread;

using namespace zezax::sensorrd;

using namespace std::chrono_literals;

using chrono::system_clock;
using std::string;
using std::string_view;
using std::to_string;
using std::vector;

namespace {

bool endswith(string_view str, const char *tail) {
  string_view tv = tail;
  if (str.size() < tv.size())
    return false;
  string_view suf(str.data() + (str.size() - tv.size()), tv.size());
  return (suf == tv);
}


vector<string> makeRrdKeys(ContextPtrT ctx) {
  vector<string> rrdKeys;
  {
    std::set<string> seen;
    for (TupleIterT tpi(ctx); tpi; ++tpi) {
      TupleT tp = tpi.getTuple();
      string name = tp.name_;
      int n = 0;
      while (seen.count(name) > 0)
        name = tp.name_ + '_' + to_string(n++);
      seen.insert(name);
      rrdKeys.emplace_back(std::move(name));
    }
  }
  return rrdKeys;
}

} // anonymous

int main(int argc, char **argv) {
  (void) argc; // FIXME rrdpath
  (void) argv;

  ContextPtrT ctx = std::make_shared<ContextT>();

  // one-time pass at startup to derive the rrd keys in order
  vector<string> rrdKeys = makeRrdKeys(ctx);

  chrono::nanoseconds period = 5s; //FIXME
  auto when = system_clock::now();

  for (int i = 0; i < 2; ++i) { // FIXME quitflag

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
        if (!key.empty())
          std::cout << "    " << key << '=' << val
                    << (alarm ? " [ALARM]" : "") << std::endl;
      }
    }

    for (AllIterT all(ctx); all; ++all) {
      std::cout << ChipT::toString(all.getChip()) << ':'
                << all.getLabel() << ':'
                << all.getName() << '='
                << all.getVal() << std::endl;
    }

    int rr = 0;
    string tmpl;
    string vals = "N";
    for (TupleIterT tpi(ctx); tpi; ++tpi, ++rr) {
      TupleT t = tpi.getTuple();
      std::cout << t.chip_ << ':'
                << t.label_ << ':'
                << t.name_ << '='
                << t.val_ << ' '
                << (t.alarm_ ? " [ALARM]" : "")
                << std::endl;
      if (!tmpl.empty())
        tmpl += ':';
      tmpl += rrdKeys[rr];
      vals += ':';
      vals += to_string(t.val_);
      std::cout << "rrdtool update RRD -t " << tmpl << ' ' << vals << std::endl;
    }

    when += period;
    this_thread::sleep_until(when);
  }
  return 0;
}
