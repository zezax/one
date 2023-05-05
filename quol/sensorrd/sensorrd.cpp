// sensorrd.cpp - main file

#include <chrono>
#include <set>
#include <thread>
#include <vector>

#include <rrd.h>

#include "rrdb.h"
#include "sensor.h"
#include "log.h"
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
    logErr("Usage: sensorrd <rrdPath>");
    return 1;
  }
  string rrdPath = argv[1];

  openlog("sensorrd", 0, LOG_DAEMON);

  ContextPtrT ctx = std::make_shared<ContextT>();

  // one-time pass at startup to derive the rrd keys in order
  vector<string> rrdKeys = makeRrdKeys(ctx);

  if (!pathExists(rrdPath)) {
    rrdCreate(rrdPath, rrdKeys);
    logSys(LOG_NOTICE, "created rrd ", rrdPath);
  }

  chrono::nanoseconds period = 60s;
  auto when = system_clock::now();
  int minutes = 0;

  for (;;) { // FIXME quitflag
    int rr = 0;
    string tmpl;
    string vals = "N";
    for (TupleIterT tpi(ctx); tpi; ++tpi, ++rr) {
      TupleT t = tpi.getTuple();

      if (t.alarm_) {
        logSys(LOG_ERR, "Sensor alarm: Chip ", t.chip_, ": ",
               t.name_, ": ", t.val_, " [ALARM]");
      }
      if (minutes == 0) {
        logSys(LOG_INFO, t.chip_, ": ", t.label_, ": ", t.name_, ": ", t.val_);
        minutes = 0;
      }

      tmpl += rrdKeys[rr];
      tmpl += ':';
      vals += ':';
      append(vals, t.val_);
    }
    if (!tmpl.empty())
      tmpl.erase(tmpl.end() - 1);

    if (!rrdUpdate(rrdPath, tmpl, vals))
      logSys(LOG_ERR, "failed to update rrd ", tmpl, ' ', vals);

    when += period;
    this_thread::sleep_until(when);
    if (++minutes >= 30)
      minutes = 0;
  }
  return 0;
}
