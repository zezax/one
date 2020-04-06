// config.h

#pragma once

#include <map>
#include <memory>
#include <string>

#include "actor.h"
#include "queue.h"
#include "tailer.h"

namespace flume {

class ConfigT {
public:
  ConfigT();
  ~ConfigT();

  void readCfg(const char *path);
  void start();
  void stop();

  QueueT<ActionT *> &getQueue() { return queue_; }
  const std::string *getSetting(const char *key);
  const std::string *getSetting(const std::string &key);
  bool getString(const char *key, std::string &out);
  bool getString(const std::string &key, std::string &out);
  bool getInt(const char *key, int &out);
  bool getInt(const std::string &key, int &out);
  bool getUlong(const char *key, unsigned long &out);
  bool getUlong(const std::string &key, unsigned long &out);
  bool getDouble(const char *key, double &out);
  bool getDouble(const std::string &key, double &out);

protected:
  std::string                         cfgPath_;
  QueueT<ActionT *>                   queue_;
  std::unique_ptr<ActorT>             actor_;
  std::map<std::string, TailerT>      log2tailer_;
  std::map<std::string, std::string>  settings_;
};

}
