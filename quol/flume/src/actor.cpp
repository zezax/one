// actor.cpp

#include "actor.h"

#include <unistd.h>

#include <iostream>

#include "config.h"


using std::cout;
using std::endl;
using std::string;
using std::unique_ptr;

namespace flume {

ActorT::ActorT(ConfigT *cfg)
: cfg_(cfg),
  debug_(0)
{
}


ActorT::~ActorT()
{
  reqStop();
  if (thr_.joinable())
    thr_.join();
}


void
ActorT::registerSink(const std::string &key, unique_ptr<SinkBaseT> sink)
{
  auto it = key2sink_.find(key);
  if (it != key2sink_.end())
    throw std::invalid_argument("duplicate sink key");
  key2sink_.emplace(key, std::move(sink));
}


void
ActorT::start()
{
  cfg_->getInt("actor.debug", debug_);
  thr_ = std::thread(&ActorT::run, this);
  for (auto& pr : key2sink_) {
    pr.second->start();
  }
}


void
ActorT::reqStop()
{
  cfg_->getQueue().enqueue(nullptr); // poison pill
}


void
ActorT::run()
{
  cout << "actor begin" << endl;

  for (;;) {
    unique_ptr<ActionT> act(cfg_->getQueue().dequeue());
    if (!act)
      break;
    if (debug_ >= 1) {
      cout << "ACTOR:";
      for (const string &arg : act->args_)
        cout << ' ' << arg;
      cout << endl;
    }
    string &key = act->args_[1];
    auto it = key2sink_.find(key);
    if (it == key2sink_.end())
      cout << "no sink registered for key " << key << endl;
    else
      it->second->handle(act.get());
  }

  cout << "actor exit" << endl;
}

}
