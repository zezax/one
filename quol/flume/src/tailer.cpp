#include "tailer.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <string.h>

#include <chrono>
#include <iostream>

#include "config.h"

namespace chrono = std::chrono;
namespace this_thread = std::this_thread;

using namespace std::chrono_literals;

using std::cout;
using std::endl;
using std::string;
using std::unique_ptr;
using std::vector;

namespace flume {

TailerT::TailerT(ConfigT *cfg, const string &path)
: cfg_(cfg),
  path_(path),
  ptr_(nullptr),
  end_(nullptr),
  bufSize_(8192),
  off_(0),
  inode_(0),
  fd_(-1),
  quitReq_(false)
{
}


TailerT::~TailerT()
{
  quitReq_ = true;
  if (thr_.joinable())
    thr_.join();
}


void
TailerT::start()
{
  cfg_->getUlong("tailer.bufsize", bufSize_);
  if ((bufSize_ < 1) || (bufSize_ > (32 * 1048576)))
    throw std::invalid_argument("invalid tailer.bufsize");
  buf_.reserve(bufSize_);
  ptr_ = &buf_[0];
  end_ = &buf_[0];

  if (quitReq_)
    return;
  thr_ = std::thread(&TailerT::run, this);
}


void
TailerT::reqStop()
{
  quitReq_ = true;
}


void
TailerT::addTrigger(const string &pat, const vector<string> &args)
{
  triggers_.emplace_back(pat, args);
}


void
TailerT::run() {
  cout << "tailing " << path_ << endl;

  string buf;
  buf.reserve(bufSize_);

  while (!quitReq_) {
    readLine(buf);
    for (auto &trigger : triggers_) {
      MatchT mat;
      if (trigger.matches(buf, mat)) {
        unique_ptr<ActionT> act = std::make_unique<ActionT>();
        act->args_.push_back(buf); // whole line first
        trigger.appendArgs(act->args_, mat);
        cfg_->getQueue().enqueue(act.release());
	break; 			// first match wins
      }
    }
  }

  cout << "finished " << path_ << endl;
}


void
TailerT::readLine(string &str)
{
  str.clear();
  if (ptr_ >= end_) {
    waitForData();
    if (quitReq_)
      return;
  }
  readRawLine(str);
}


void
TailerT::waitForData()
{
  int tries = 0;
  struct stat st;

  while (!quitReq_) {
    if (fd_ < 0) {
      doOpen();
      if (fd_ < 0)
        break;
      fstat(fd_, &st);
      if (inode_ == 0) {        // first open
        off_ = st.st_size;
        string scratch;
        scratch.reserve(bufSize_);
        readRawLine(scratch);   // skip partial line
      }
      else if (inode_ != st.st_ino) { // subsequent file
        off_ = 0;                     // read from start
      }
      inode_ = st.st_ino;
    }

    fstat(fd_, &st);
    if (st.st_size != off_)
      break;

    if (++tries > 10) {
      tries = 0;
      doClose();
    }
    else
      this_thread::sleep_for(1s);
  }
}


void
TailerT::readRawLine(string &str)
{
  while (str.size() < str.capacity()) {
    if (ptr_ >= end_)
      fillBuf();
    if (ptr_ >= end_)
      break;

    ++off_;
    char ch = *ptr_++;
    if (ch == '\n')             // don't include newline
      break;
    str += ch;
  }

  if (!str.empty() && (str.back() == '\r'))
    str.pop_back();
}


void
TailerT::fillBuf()
{
  char *buf = &buf_[0];
  if (ptr_ < end_) {
    size_t nb = end_ - ptr_;
    memmove(buf, ptr_, nb);
    end_ = buf + nb;
  }
  else
    end_ = buf;
  ptr_ = buf;
  size_t room = buf + buf_.capacity() - end_;
  ssize_t got = pread(fd_, end_, room, off_);  // !!! blocking io
  if (got < 0) {
    close(fd_);
    fd_ = -1;
  }
  else
    end_ += got;
}


void
TailerT::doOpen()
{
  while (!quitReq_) {
    fd_ = open(path_.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd_ >= 0)
      return;
    this_thread::sleep_for(1s);
  }
}


void
TailerT::doClose()
{
  close(fd_);
  fd_ = -1;
  ptr_ = &buf_[0];
  end_ = &buf_[0];
}

}
