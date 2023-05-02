// sink.cpp

#include "sink.h"

#include "config.h"
#include "rrd.h"

#include <sys/utsname.h>
#include <sys/wait.h>

#include <chrono>
#include <iostream> // FIXME

namespace chrono = std::chrono;

using std::cout; // FIXME
using std::endl; // FIXME
using std::string;
using std::unique_lock;
using std::vector;

///////////////////////////////////////////////////////////////////////////////

namespace {

void
fullWrite(int fd, const string &str)
{
  const char *ptr = str.data();
  size_t len = str.size();
  while (len) {
    ssize_t nb = write(fd, ptr, len);
    if (nb >= 0) {
      ptr += nb;
      len -= nb;
    }
    else if (errno != EINTR)
      break;
  }
}

}

///////////////////////////////////////////////////////////////////////////////

namespace flume {

void SinkThreadT::stop() {
  {
    unique_lock<std::mutex> lock(mtx_);
    quitReq_ = true;
    cond_.notify_all();
  }
  if (thr_.joinable()) {
    thr_.join();
  }
}

///////////////////////////////////////////////////////////////////////////////

SinkIgnoreT::SinkIgnoreT(ConfigT *cfg) : SinkBaseT(cfg) { }

void SinkIgnoreT::start() { }

void SinkIgnoreT::handle(const ActionT *) { }

///////////////////////////////////////////////////////////////////////////////

SinkRrdT::SinkRrdT(ConfigT *cfg)
: SinkThreadT(cfg),
  prefix_("/var/log/"),
  interval_(60000),
  debug_(0)
{
}


SinkRrdT::~SinkRrdT() {
  stop();
}


void
SinkRrdT::start()
{
  cfg_->getString("rrd.prefix", prefix_);
  cfg_->getInt("rrd.debug", debug_);
  double dd;
  if (cfg_->getDouble("rrd.interval", dd)) {
    if (dd < 0.1)
      throw std::invalid_argument("invalid rrd.interval");
    interval_ = chrono::milliseconds(static_cast<long>(dd * 1000.0));
  }
  thr_ = std::thread(&SinkRrdT::run, this);
}


void
SinkRrdT::handle(const ActionT *act)
{
  size_t len = act->args_.size();
  if (len < 4) {
    cout << "RRD: too few parameters" << endl;
    return;
  }
  if (debug_ >= 1)
    cout << "RRD: " << act->args_[0] << endl;
  const string rrd = prefix_ + act->args_[2];
  const string &name = act->args_[3];
  const string &val = (len > 4) ? act->args_[4] : "1";
  {
    unique_lock<std::mutex> lock(mtx_);
    StateT &state = rrd2state_[rrd];
    ScalarT &scalar = state.name2val_[name];
    scalar.add(val);
    if (debug_ >= 2)
      cout << rrd << ':' << name << ':' << val << ':'
           << scalar.to_string() << endl;
  }
}


void
SinkRrdT::run()
{
  cout << "rrd update thread start" << endl;
  unique_lock<std::mutex> lock(mtx_);

  while (!quitReq_) {
    cond_.wait_for(lock, interval_);
    if (quitReq_)
      break;
    for (auto &pr : rrd2state_)
      updateRrd(pr.first, pr.second);
  }

  cout << "rrd update thread exit" << endl;
}


void
SinkRrdT::updateRrd(const string &rrd, StateT &state)
{
  // called with lock held
  string templ;
  string vals = "N";
  for (auto &pr : state.name2val_) {
    if (!templ.empty())
      templ += ':';
    templ += pr.first;
    vals += ':';
    vals += pr.second.to_string();
  }
  vector<string> args;
  args.push_back("update");
  args.push_back(rrd);
  args.push_back("-t");
  args.push_back(templ);
  args.push_back(vals);
  vector<const char *> argv;
  for (string &s : args) {
    argv.push_back(s.c_str());
  }
  if (debug_ >= 3)
    cout << "rrdtool update " << rrd << " -t " << templ << ' ' << vals << endl;

  rrd_clear_error();
  int res = rrd_update(argv.size(), const_cast<char **>(argv.data()));
  if (res < 0)
    cout << "rrd_update: " << rrd_get_error()
         << ' ' << templ << ' ' << vals << endl;
}

///////////////////////////////////////////////////////////////////////////////

SinkMailT::SinkMailT(ConfigT *cfg)
: SinkThreadT(cfg),
  sleep_(10000),
  interval_(300000),
  limit_(100),
  debug_(0)
{
}


SinkMailT::~SinkMailT() {
  stop();
}


void
SinkMailT::start()
{
  // get settings from configuration
  cfg_->getInt("mail.debug", debug_);

  cfg_->getUlong("mail.limit", limit_);
  if (limit_ < 1)
    throw std::invalid_argument("invalid mail.limit");

  double dd;
  if (cfg_->getDouble("mail.sleep", dd)) {
    if (dd < 0.1)
      throw std::invalid_argument("invalid mail.sleep");
    sleep_ = chrono::milliseconds(static_cast<long>(dd * 1000.0));
  }
  if (cfg_->getDouble("mail.interval", dd)) {
    if (dd < 0.1)
      throw std::invalid_argument("invalid mail.interval");
    interval_ = chrono::milliseconds(static_cast<long>(dd * 1000.0));
  }

  // run the thread
  thr_ = std::thread(&SinkMailT::run, this);
}


void
SinkMailT::handle(const ActionT *act)
{
  size_t len = act->args_.size();
  if (len < 5) {
    cout << "MAIL: too few parameters" << endl;
    return;
  }
  auto ticks = chrono::system_clock::now();
  time_t now = chrono::system_clock::to_time_t(ticks);
  const string &addr = act->args_[2];
  const string &subj = act->args_[3];
  const string &line = act->args_[4];
  if (debug_ >= 1)
    cout << "MAIL: " << line << endl;
  string key = addr + ':' + subj;
  {
    unique_lock<std::mutex> lock(mtx_);
    DestT &dest = key2dest_[key];
    if (dest.email_.empty()) {
      dest.email_ = addr;
      dest.subject_ = subj;
    }
    dest.msgs_.emplace_back(now, line);
  }
}


void
SinkMailT::run()
{
  cout << "mail thread start" << endl;
  unique_lock<std::mutex> lock(mtx_);

  while (!quitReq_) {
    cond_.wait_for(lock, sleep_);

    auto ticks = chrono::system_clock::now();
    if (!quitReq_) {
      ticks -= interval_;
    }
    time_t longAgo = chrono::system_clock::to_time_t(ticks);

    for (auto &item : key2dest_) {
      DestT &dest = item.second;
      if (dest.msgs_.empty()) {
	continue;
      }

      if ((dest.msgs_.size() > limit_) ||
	  (dest.msgs_[0].arrival_ <= longAgo)) {
	vector<MsgT> vec;
	dest.msgs_.swap(vec);
	lock.unlock();
	try {
	  sendMail(dest.email_, dest.subject_, vec);
	}
	catch (const std::exception &ex) {
	  cout << ex.what() << endl;
	}
	lock.lock();              // cond wait expects lock held
      }
    }
  }

  cout << "mail thread exit" << endl;
}


void
SinkMailT::sendMail(const string &addr, const string &subj,
		    const vector<MsgT> &msgs)
{
  int fds[2];                   // 0=read 1=write
  if (pipe(fds) < 0)
    throw std::system_error(errno, std::system_category(),
                            "failed to create pipe");

  pid_t pid = fork();
  if (pid < 0) {
    close(fds[0]);
    close(fds[1]);
    throw std::system_error(errno, std::system_category(), "fork failed");
  }
  if (pid == 0) {               // child
    close(fds[1]);              // close write end
    execMail(fds[0], addr, subj);
    _exit(1);
  }

  // parent
  close(fds[0]);                // close read end
  composeMail(fds[1], addr, msgs);
  close(fds[1]);                // done writing
  int status = 0;
  pid_t res = waitpid(pid, &status, 0);
  if (res < 0)
    throw std::system_error(errno, std::system_category(), "waitpid failed");
  if (!WIFEXITED(status) || (WEXITSTATUS(status) != 0))
    throw std::runtime_error("failed to send mail");
}


void
SinkMailT::execMail(int fd, const string &addr, const string &subj)
{
  dup2(fd, STDIN_FILENO);
  close(fd);

  struct utsname uts;
  uname(&uts);
  string sub = subj;
  sub += ' ';
  sub += uts.nodename;

  vector<string> args;
  args.push_back("/usr/bin/mailx");
  args.push_back("-s");
  args.push_back(std::move(sub));
  args.push_back(addr);
  vector<const char *> argv;
  for (string &s : args)
    argv.push_back(s.c_str());
  argv.push_back(nullptr);      // terminator
  execve(argv[0], const_cast<char **>(argv.data()), environ);
}


void
SinkMailT::composeMail(int fd, const string &addr, const vector<MsgT> &msgs)
{
  if (msgs.empty())
    return;

  struct tm stm;
  localtime_r(&msgs[0].arrival_, &stm);
  string buf;
  buf.resize(512);
  size_t nb = strftime(&buf[0], buf.size(),
                       "Date: %A, %B %d, %Y, %I:%M:%S %p", &stm);
  buf.resize(nb);

  char host[256];
  char dom[256];
  gethostname(host, sizeof(host));
  getdomainname(dom, sizeof(dom));
  if (debug_ >= 2)
    cout << "Mailing " << addr << ": " << host << '.' << dom
         << ' ' << buf << endl;
  buf += "\nHost: ";
  buf += host;
  buf += '.';
  buf += dom;
  buf += "\n\n";

  for (const MsgT &msg : msgs) {
    buf += msg.text_;
    buf += '\n';
  }

  if (debug_ >= 3)
    cout << buf << endl;
  fullWrite(fd, buf);
}

}
