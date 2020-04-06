// config.cpp

#include "config.h"

#include <fstream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "actor.h"
#include "sink.h"
#include "tailer.h"

using std::string;
using std::unique_ptr;
using std::vector;
using std::thread;

namespace flume {

ConfigT::ConfigT()
: queue_(1024)
{
  actor_.reset(new ActorT(this));
}


ConfigT::~ConfigT()
{
  stop();
}


void
ConfigT::readCfg(const char *path)
{
  cfgPath_ = path;
  string tok;
  string line;
  vector<string> toks;
  std::ifstream input(cfgPath_);

  while (std::getline(input, line)) {
    if (line.empty() || (line.front() == '#'))
      continue;
    toks.clear();
    std::istringstream strm(line);
    while (std::getline(strm, tok, '\t'))
      toks.push_back(tok);

    string verb = std::move(toks[0]);
    toks.erase(toks.begin());
    if (verb == "set") {
      if (toks.size() != 2)
	throw std::invalid_argument("set requires 2 arguments");
      settings_.emplace(std::move(toks[0]), std::move(toks[1]));
    }
    else if (verb == "scan") {
      if (toks.size() < 3)
	throw std::invalid_argument("scan requires 3 arguments");
      string log = std::move(toks[0]);
      toks.erase(toks.begin());
      auto it = log2tailer_.find(log);
      if (it == log2tailer_.end()) {
        auto pr = log2tailer_.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(log),
            std::forward_as_tuple(this, log));
        it = pr.first;
      }
      string pat = std::move(toks[0]);
      toks.erase(toks.begin());
      it->second.addTrigger(pat, toks);
    }
  }
}


void
ConfigT::start()
{
  actor_->registerSink("ignore",
                       unique_ptr<SinkIgnoreT>(new SinkIgnoreT(this)));
  actor_->registerSink("rrd", unique_ptr<SinkRrdT>(new SinkRrdT(this)));
  actor_->registerSink("mail", unique_ptr<SinkMailT>(new SinkMailT(this)));
  actor_->start();
  for (auto &it : log2tailer_)
    it.second.start();
}


void
ConfigT::stop()
{
  for (auto &it : log2tailer_)
    it.second.reqStop();
  log2tailer_.clear();
  if (actor_)
    actor_->reqStop();
  actor_.reset(nullptr);
}


const string *
ConfigT::getSetting(const char *key)
{
  string skey = key;
  return getSetting(skey);
}


const string *
ConfigT::getSetting(const string &key)
{
  const auto &pr = settings_.find(key);
  if (pr == settings_.end())
    return nullptr;
  return &pr->second;
}


bool
ConfigT::getString(const char *key, string &out)
{
  string skey = key;
  return getString(skey, out);
}


bool
ConfigT::getString(const string &key, string &out)
{
  const string *sptr = getSetting(key);
  if (!sptr)
    return false;
  out = *sptr;
  return true;
}


bool
ConfigT::getInt(const char *key, int &out)
{
  string skey = key;
  return getInt(skey, out);
}


bool
ConfigT::getInt(const string &key, int &out)
{
  const string *sptr = getSetting(key);
  if (!sptr)
    return false;
  try {
    out = std::stoi(*sptr);
  }
  catch (...) {
    return false;
  }
  return true;
}


bool
ConfigT::getUlong(const char *key, unsigned long &out)
{
  string skey = key;
  return getUlong(skey, out);
}


bool
ConfigT::getUlong(const string &key, unsigned long &out)
{
  const string *sptr = getSetting(key);
  if (!sptr)
    return false;
  try {
    out = std::stoul(*sptr);
  }
  catch (...) {
    return false;
  }
  return true;
}


bool
ConfigT::getDouble(const char *key, double &out)
{
  string skey = key;
  return getDouble(skey, out);
}


bool
ConfigT::getDouble(const string &key, double &out)
{
  const string *sptr = getSetting(key);
  if (!sptr)
    return false;
  try {
    out = std::stod(*sptr);
  }
  catch (...) {
    return false;
  }
  return true;
}

}
