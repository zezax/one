// sink.h

#pragma once

#include <chrono>
#include <condition_variable>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "action.h"
#include "scalar.h"

namespace flume {

class ConfigT;


class SinkBaseT {
public:
  SinkBaseT(ConfigT *cfg) : cfg_(cfg) { }
  virtual ~SinkBaseT() { }
  virtual void start() = 0;
  virtual void handle(const ActionT *act) = 0;

protected:
  ConfigT  *cfg_;
};


class SinkThreadT : public SinkBaseT {
public:
  SinkThreadT(ConfigT *cfg) : SinkBaseT(cfg), quitReq_(false) { }

protected:
  void stop();

  std::thread              thr_;
  std::mutex               mtx_;
  std::condition_variable  cond_;
  volatile bool            quitReq_;
};


class SinkIgnoreT : public SinkBaseT {
public:
  SinkIgnoreT(ConfigT *cfg);
  virtual void start() override;
  virtual void handle(const ActionT *act) override;
};


class SinkRrdT : public SinkThreadT {
public:
  SinkRrdT(ConfigT *cfg);
  virtual ~SinkRrdT() override;
  virtual void start() override;
  virtual void handle(const ActionT *act) override;

  struct StateT {
    std::map<std::string, ScalarT>  name2val_;
  };

protected:
  void run();
  void updateRrd(const std::string &rrd, StateT &state);

  std::map<std::string, StateT>  rrd2state_;
  std::string                    prefix_;
  std::chrono::nanoseconds       interval_;
  int                            debug_;
};


class SinkMailT : public SinkThreadT {
public:
  SinkMailT(ConfigT *cfg);
  virtual ~SinkMailT() override;
  virtual void start() override;
  virtual void handle(const ActionT *act) override;

  struct MsgT {
    std::chrono::system_clock::time_point  arrival_;
    std::string                            text_;
    MsgT(std::chrono::system_clock::time_point t, const std::string &s)
      : arrival_(t), text_(s) { }
  };

  struct DestT {
    std::string        email_;
    std::string        subject_;
    std::vector<MsgT>  msgs_;
  };

protected:
  void run();
  void sendMail(const std::string &addr, const std::string &subj,
		const std::vector<MsgT> &msgs);
  void execMail(int fd, const std::string &addr, const std::string &subj);
  void composeMail(int fd, const std::string &addr,
		   const std::vector<MsgT> &msgs);

  std::map<std::string, DestT>  key2dest_;
  std::chrono::nanoseconds      sleep_;
  std::chrono::nanoseconds      interval_;
  size_t                        limit_;
  int                           debug_;
};

}
