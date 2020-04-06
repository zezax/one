// tailer.h

#pragma once

#include <string>
#include <vector>
#include <thread>

#include "actor.h"
#include "trigger.h"

namespace flume {

class ConfigT;


class TailerT {
public:
  TailerT(ConfigT *cfg, const std::string &path);
  ~TailerT();

  TailerT(const TailerT &) = delete;
  TailerT &operator=(const TailerT &) = delete;

  void start();
  void reqStop();
  void addTrigger(const std::string &pat,
                  const std::vector<std::string> &args);

protected:
  void run();
  void readLine(std::string &buf);
  void readRawLine(std::string &buf);
  void waitForData();
  void fillBuf();
  void doOpen();
  void doClose();

  std::thread             thr_;
  ConfigT                *cfg_;
  std::string             path_;
  std::vector<TriggerT>   triggers_;
  std::string             buf_;
  char                   *ptr_;
  char                   *end_;
  size_t                  bufSize_;
  off_t                   off_;
  ino_t                   inode_;
  int                     fd_;
  volatile bool           quitReq_;
};

}
