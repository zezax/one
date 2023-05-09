// sensorrd.cpp - main file

#include <signal.h>
#include <stdlib.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <set>
#include <vector>

#include <rrd.h>

#include "rrdb.h"
#include "sensor.h"
#include "log.h"
#include "util.h"

namespace chrono = std::chrono;

using namespace std::chrono_literals;
using namespace zezax::sensorrd;

using chrono::system_clock;
using std::condition_variable;
using std::memory_order_relaxed;
using std::string;
using std::string_view;
using std::to_string;
using std::unique_lock;
using std::vector;

namespace {

std::atomic<bool>  gQuitReq = false;
std::mutex         gMutex;
condition_variable gCond;


void handleSig(int) {
  gQuitReq.store(true, memory_order_relaxed);
  unique_lock<std::mutex> lock(gMutex);
  gCond.notify_all();
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

  rrdKeys.emplace_back("loadavg");
  return rrdKeys;
}

} // anonymous


int main(int argc, char **argv) {
  string rrdPath;
  if (argc == 2)
    rrdPath = argv[1];
  else if (argc > 2) {
    logErr("Usage: sensorrd <rrdPath>");
    return 1;
  }

  openlog("sensorrd", 0, LOG_DAEMON);
  logSys(LOG_NOTICE, "begin");

  ContextPtrT ctx = std::make_shared<ContextT>();

  // one-time pass at startup to derive the rrd keys in order
  vector<string> rrdKeys;
  if (rrdPath.empty())
    logSys(LOG_INFO, "not logging to any rrd");
  else {
    rrdKeys = makeRrdKeys(ctx);
    if (pathExists(rrdPath))
      logSys(LOG_INFO, "logging to rrd ", rrdPath);
    else{
      rrdCreate(rrdPath, rrdKeys);
      logSys(LOG_NOTICE, "created rrd ", rrdPath);
    }
  }

  chrono::nanoseconds period = 60s;
  auto when = system_clock::now();
  int minutes = 0;

  unique_lock<std::mutex> lock(gMutex);
  signal(SIGINT, handleSig);
  signal(SIGQUIT, handleSig);
  signal(SIGTERM, handleSig);

  while (!gQuitReq.load(memory_order_relaxed)) {
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

      if (!rrdPath.empty()) {
        concat(tmpl, rrdKeys[rr], ':');
        concat(vals, ':', t.val_);
      }
    }

    if (!rrdPath.empty()) {
      double load;
      if (getloadavg(&load, 1) > 0) {
        concat(tmpl, rrdKeys[rr], ':');
        concat(vals, ':', load);
      }
      if (!tmpl.empty())
        tmpl.erase(tmpl.end() - 1);

      if (!rrdUpdate(rrdPath, tmpl, vals))
        logSys(LOG_ERR, "failed to update rrd ", tmpl, ' ', vals);
    }

    when += period;
    gCond.wait_until(lock, when);
    if (++minutes >= 30)
      minutes = 0;
  }

  logSys(LOG_NOTICE, "exiting");
  return 0;
}
