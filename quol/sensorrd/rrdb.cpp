// rrdb.cpp - wrapper around rrd api

#include "rrdb.h"

#include <stdexcept>

#include "util.h"

namespace zezax::sensorrd {

using std::runtime_error;
using std::string;
using std::vector;


namespace {

void guessMinMax(const string &key, string &minStr, string &maxStr) {
  if (startsWith(key, "temp")) { // celsius
    minStr = "-100";
    maxStr = "250";
  }
  else if (startsWith(key, "fan")) { // rpm
    minStr = "0";
    maxStr = "12000";
  }
  else if (startsWith(key, "in")) { // volts
    minStr = "-25";
    maxStr = "25";
  }
  else {
    minStr = "NaN";
    maxStr = "NaN";
  }
}

} // anonymous


void rrdCreate(const string &rrdPath, const vector<string> &rrdKeys) {
  vector<string> args;
  args.emplace_back("create");
  args.emplace_back(rrdPath);
  args.emplace_back("-s");
  args.emplace_back("60");

  string minStr;
  string maxStr;
  for (const string &key : rrdKeys) {
    guessMinMax(key, minStr, maxStr);

    string ds = "DS:";
    ds += key + ":GAUGE:300:" + minStr + ':' + maxStr;
    args.emplace_back(std::move(ds));
  }

  args.emplace_back("RRA:AVERAGE:0.5:1:1800"); // 30 hours of minutes
  args.emplace_back("RRA:AVERAGE:0.5:5:2880"); // 10 days of 5 minutes
  args.emplace_back("RRA:AVERAGE:0.5:60:960"); // 40 days of hours
  args.emplace_back("RRA:AVERAGE:0.5:1440:3653"); // 10 years of days

  vector<const char *> argv;
  for (string &s : args)
    argv.emplace_back(s.c_str());

  rrd_clear_error();
  int res = rrd_create(static_cast<int>(argv.size()),
                       const_cast<char **>(argv.data()));
  if (res < 0)
    throw runtime_error(rrd_get_error());
}


bool rrdUpdate(const string &rrdPath,
               const string &templat,
               const string &values) {
  vector<string> args;
  args.emplace_back("update");
  args.emplace_back(rrdPath);
  args.emplace_back("-t");
  args.emplace_back(templat);
  args.emplace_back(values);

  vector<const char *> argv;
  for (string &s : args)
    argv.emplace_back(s.c_str());

  rrd_clear_error();
  int res = rrd_update(static_cast<int>(argv.size()),
                       const_cast<char **>(argv.data()));
  return (res == 0);
}

} // namespace zezax::sensorrd
