// actor.h

#pragma once

#include <map>
#include <string>
#include <thread>
#include <vector>

#include "action.h"
#include "sink.h"

namespace flume {

class ConfigT;


class ActorT {
public:
  ActorT(ConfigT *cfg);
  ~ActorT();

  ActorT(const ActorT &) = delete;
  ActorT &operator=(const ActorT &) = delete;

  void registerSink(const std::string &key, std::unique_ptr<SinkBaseT> sink);

  void start();
  void reqStop();

protected:
  void run();

  std::thread                                         thr_;
  ConfigT                                            *cfg_;
  std::map<std::string, std::unique_ptr<SinkBaseT>>   key2sink_;
  int                                                 debug_;
};

}
