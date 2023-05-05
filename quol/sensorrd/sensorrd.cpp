// sensorrd.cpp - main file

#include <chrono>
#include <iostream>
#include <set>
#include <thread>
#include <vector>

#include <rrd.h>

#include "rrdb.h"
#include "sensor.h"
#include "util.h"

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
  if (argc != 2) {
    std::cerr << "Usage: sensorrd <rrdPath>" << std::endl;
    return 1;
  }
  string rrdPath = argv[1];

  ContextPtrT ctx = std::make_shared<ContextT>();

  // one-time pass at startup to derive the rrd keys in order
  vector<string> rrdKeys = makeRrdKeys(ctx);

  if (!pathExists(rrdPath)) {
    rrdCreate(rrdPath, rrdKeys);
    std::cout << "created rrd " << rrdPath << std::endl;
  }

  chrono::nanoseconds period = 60s;
  auto when = system_clock::now();

  for (;;) { // FIXME quitflag
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
      tmpl += rrdKeys[rr];
      tmpl += ':';
      vals += ':';
      vals += to_string(t.val_);
    }
    if (!tmpl.empty())
      tmpl.erase(tmpl.end() - 1);

    if (!rrdUpdate(rrdPath, tmpl, vals))
      std::cout << "failed to update rrd " << tmpl << ' ' << vals << std::endl;

    when += period;
    this_thread::sleep_until(when);
  }
  return 0;
}
