// rrdb.h - wrapper around rrd api

#pragma once

#include <string>
#include <vector>

#include <rrd.h>

namespace zezax::sensorrd {

void rrdCreate(const std::string              &rrdPath,
               const std::vector<std::string> &rrdKeys);

bool rrdUpdate(const std::string &rrdPath,
               const std::string &templat,
               const std::string &values);

} // namespace zezax::sensorrd
