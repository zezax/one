// loggle.cpp
//
// TODO: change target, pidfn, logfn
//
///////////////////////////////////////////////////////////////////////////////

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <errno.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <regex.h>
#include <pthread.h>

#include <exception>
#include <new>


static volatile bool gQuitReq = false;


static void
usage()
{
  printf("Usage: loggle <options>\n");
  printf("  -i          ignore case when matching (must come first)\n");
  printf("  -e <regex>  pattern to look for (must supply one or more)\n");
  printf("  -m <email>  address to receive alerts (required)\n");
  printf("  -s <str>    subject of email (def: LOGGLE)\n");
  printf("  -f <path>   file to watch (def: /var/log/syslog)\n");
  printf("  -t <num>    seconds between email messages (def: 300)\n");
  printf("  -b <num>    maximum batch size for email (def: 100)\n");
  printf("  -h <num>    hit count before triggering alert (def: 1)\n");
  printf("  -c <num>    line count before zeroing hits (def: 2^31-1)\n");
  printf("  -fg         do not become a daemon\n");
}

///////////////////////////////////////////////////////////////////////////////
//
// MISC
//
///////////////////////////////////////////////////////////////////////////////

static void
fullWrite(int fd, const char *ptr, size_t len)
{
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


static void
printErr(const char *msg)
{
  fullWrite(STDERR_FILENO, msg, strlen(msg));
  msg = strerror(errno);
  fullWrite(STDERR_FILENO, msg, strlen(msg));
  fullWrite(STDERR_FILENO, "\n", 1);
}

///////////////////////////////////////////////////////////////////////////////
//
// TAIL READER
//
///////////////////////////////////////////////////////////////////////////////

class LgTailT {
public:
  static const size_t _bufSize = 8192;

  LgTailT() : _buf(NULL), _fd(-1) { }
  ~LgTailT() { if (_fd >= 0) close(_fd); delete[] _buf; }

  int Init(const char *path);

  ssize_t ReadLine(char *buf, size_t buflen);

protected:
  LgTailT(const LgTailT &ref);
  LgTailT &operator=(const LgTailT &ref);

  void waitForData();
  ssize_t readRawLine(char *buf, size_t len);
  void fillBuf();
  void doOpen();
  void doClose();

  const char  *_path;
  char        *_buf;
  char        *_ptr;
  char        *_end;
  off_t        _off;
  int          _fd;
};


int
LgTailT::Init(const char *path)
{
  _path = path;
  _buf = new char[_bufSize];
  _ptr = _buf;
  _end = _buf;
  _off = 0;
  return 0;
}


ssize_t
LgTailT::ReadLine(char *buf, size_t buflen)
{
  if (_ptr >= _end)
    waitForData();
  return readRawLine(buf, buflen);
}

///////////////////////////////////////////////////////////////////////////////

void
LgTailT::waitForData()
{
  int tries = 0;
  struct stat st;
  char buf[2048];

  while (!gQuitReq) {
    if (_fd < 0) {
      doOpen();
      if (_fd < 0)
        break;
      fstat(_fd, &st);
      _off = st.st_size;
      readRawLine(buf, sizeof(buf));  // skip partial line
    }

    fstat(_fd, &st);
    off_t siz = st.st_size;
    if (siz != _off)
      break;

    ++tries;
    if (tries > 100) {
      tries = 0;
      doClose();
    }
    else
      poll(NULL, 0, 1000);
  }
}


ssize_t
LgTailT::readRawLine(char *buf, size_t buflen)
{
  char *dst = buf;
  char *dend = buf + buflen - 2;  // for newline and null
  char ch;

  while (dst < dend) {
    if (_ptr >= _end)
      fillBuf();
    if (_ptr >= _end)
      break;

    ++_off;
    ch = *_ptr++;
    if (ch == '\n')
      break;
    *dst++ = ch;
  }

  if ((dst > buf) && (dst[-1] == '\r'))
    --dst;
  *dst++ = '\n';                // include newline
  *dst = '\0';
  return dst - buf;
}


void
LgTailT::fillBuf()
{
  if (_ptr < _end) {
    size_t nb = _end - _ptr;
    memmove(_buf, _ptr, nb);
    _end = _buf + nb;
  }
  else
    _end = _buf;
  _ptr = _buf;
  size_t room = _buf + _bufSize - _end;
  ssize_t got = pread(_fd, _end, room, _off);  // !!! blocking io
  if (got < 0) {
    close(_fd);
    _fd = -1;
  }
  else
    _end += got;
}


void
LgTailT::doOpen()
{
  while (!gQuitReq) {
    _fd = open(_path, O_RDONLY | O_CLOEXEC);
    if (_fd >= 0)
      return;
    poll(NULL, 0, 1000);
  }
}


void
LgTailT::doClose()
{
  close(_fd);
  _fd = -1;
  _ptr = _buf;
  _end = _buf;
  _off = 0;
}

///////////////////////////////////////////////////////////////////////////////
//
// MESSAGE
//
///////////////////////////////////////////////////////////////////////////////

struct LgMsgT {
  LgMsgT  *next;
  time_t   arrival;
  char     msg[1];

  LgMsgT() { next = NULL; }
  static void *operator new(size_t sz);
  static void *operator new(size_t sz, void *p) { return p; }
  static void operator delete(void *p) { free(p); }
  static LgMsgT *New(const char *str);

protected:
  LgMsgT(const LgMsgT &ref);
  LgMsgT &operator=(const LgMsgT &ref);
};


void *
LgMsgT::operator new(size_t sz)
{
  void *p = malloc(sz);
  if (!p)
    throw std::bad_alloc();
  return p;
}


LgMsgT *
LgMsgT::New(const char *str)
{
  if (!str)
    throw std::exception();
  size_t len = strlen(str);
  void *vp = malloc(sizeof(LgMsgT) + len);
  LgMsgT *rv = new(vp) LgMsgT();  // placement new to construct

  struct timeval stv;
  gettimeofday(&stv, NULL);
  rv->arrival = stv.tv_sec;

  memcpy(rv->msg, str, len + 1);
  return rv;
}

///////////////////////////////////////////////////////////////////////////////
//
// MESSAGE LIST
//
///////////////////////////////////////////////////////////////////////////////

class LgMsgListT {
public:
  LgMsgListT();
  ~LgMsgListT();

  int Init(size_t maxBatch, time_t maxLag, const char *email,
           const char *subject);

  void Append(const char *msg);

  void *Run();                  // thread calls this

protected:
  LgMsgListT(const LgMsgListT &ref);
  LgMsgListT &operator=(const LgMsgListT &ref);

  void sendMail(LgMsgT *list);
  void execMail(int fd);
  void composeMail(int fd, LgMsgT *list);
  void reset() { _tail = _head = NULL; _cnt = 0; }

  LgMsgT           *_head;
  LgMsgT           *_tail;
  const char       *_email;
  const char       *_subject;
  size_t            _cnt;
  size_t            _limit;
  time_t            _lag;
  pthread_mutex_t   _mtx;
};


LgMsgListT::LgMsgListT()
  : _head(NULL),
    _tail(NULL),
    _cnt(0)
{
  _email = "root";
  _subject = "LOGGLE";
  _limit = 100;
  _lag = 300;                   // five minutes
  pthread_mutex_init(&_mtx, NULL);
}


LgMsgListT::~LgMsgListT()
{
  pthread_mutex_lock(&_mtx);
  LgMsgT *p;
  LgMsgT *q;
  for (p = _head; p; p = q) {
    q = p->next;
    delete p;
  }
  pthread_mutex_unlock(&_mtx);
  pthread_mutex_destroy(&_mtx);
}


int
LgMsgListT::Init(size_t maxBatch, time_t maxLag, const char *email,
     const char *subject)
{
  if (maxBatch)
    _limit = maxBatch;
  if (maxLag)
    _lag = maxLag;
  if (email && *email)
    _email = email;
  if (subject && *subject)
    _subject = subject;
  return 0;
}


void
LgMsgListT::Append(const char *m)
{
  if (gQuitReq)
    return;
  LgMsgT *node = LgMsgT::New(m);
  pthread_mutex_lock(&_mtx);
  if (_tail)
    _tail->next = node;
  else
    _head = node;
  _tail = node;
  ++_cnt;
  pthread_mutex_unlock(&_mtx);
}


void *
LgMsgListT::Run()
{
  struct timeval stv;

  while (!gQuitReq) {
    poll(NULL, 0, 1000);
    gettimeofday(&stv, NULL);
    time_t longAgo = stv.tv_sec - _lag;

    pthread_mutex_lock(&_mtx);

    LgMsgT *ripe = NULL;
    if ((_cnt > _limit) || (_head && (_head->arrival < longAgo))) {
      ripe = _head;
      reset();
    }

    pthread_mutex_unlock(&_mtx);

    if (ripe)
      sendMail(ripe);
  }

  pthread_mutex_lock(&_mtx);
  if (_head) {
    sendMail(_head);            // flush last bits
    reset();
  }
  pthread_mutex_unlock(&_mtx);

  fprintf(stderr, "message list closed\n");
  return NULL;
}

///////////////////////////////////////////////////////////////////////////////

void
LgMsgListT::sendMail(LgMsgT *list)
{
  int fds[2];  // 0=read 1=write
  if (pipe(fds) < 0) {
    printErr("pipe failed: ");
    return;
  }

  pid_t pid = fork();
  if (pid < 0) {
    close(fds[0]);
    close(fds[1]);
    printErr("fork failed: ");
    return;
  }
  if (pid == 0) {               // child
    close(fds[1]);              // close write end
    execMail(fds[0]);
    _exit(1);
  }

  // parent
  close(fds[0]);              // close read end
  composeMail(fds[1], list);
  close(fds[1]);              // done writing
  waitpid(pid, NULL, 0);
}


void
LgMsgListT::execMail(int fd)
{
  dup2(fd, STDIN_FILENO);
  close(fd);

  struct utsname uts;
  uname(&uts);
  char sbuf[1024];
  snprintf(sbuf, sizeof(sbuf), "%s %s", _subject, uts.nodename);

  char *args[5];
  args[0] = "/usr/bin/mailx";
  args[1] = "-s";
  args[2] = sbuf;
  args[3] = (char *) _email;
  args[4] = NULL;
  execve(args[0], args, environ);
  printErr("execve failed: ");
}


void
LgMsgListT::composeMail(int fd, LgMsgT *list)
{
  char buf[512];
  struct tm stm;
  localtime_r(&list->arrival, &stm);
  size_t nb = strftime(buf, sizeof(buf),
                       "Date: %A, %B %d, %Y, %I:%M:%S %p\n", &stm);
  fullWrite(fd, buf, nb);

  char hbuf[256];
  char dbuf[256];
  gethostname(hbuf, sizeof(hbuf));
  getdomainname(dbuf, sizeof(dbuf));
  fprintf(stderr, "Mailing %s: %s.%s %s", _email, hbuf, dbuf, buf);
  nb = snprintf(buf, sizeof(buf), "Host: %s.%s\n\n", hbuf, dbuf);
  fullWrite(fd, buf, nb);

  LgMsgT *p;
  LgMsgT *q;
  for (p = list; p; p = q) {
    q = p->next;
    fullWrite(fd, p->msg, strlen(p->msg));
    delete p;
  }
}

///////////////////////////////////////////////////////////////////////////////
//
// MAIN
//
///////////////////////////////////////////////////////////////////////////////

static void
handleSig(int)
{
  gQuitReq = true;
}


static void *
runner(void *arg)
{
  LgMsgListT *mlist = (LgMsgListT *) arg;
  try {
    return mlist->Run();
  }
  catch (...) {
    printf("thread caught exception\n");
  }
  return NULL;
}


static int
daemonize(const char *pidPath)
{
  pid_t pid = fork();
  if (pid < 0) {
    printErr("fork #1 failed: ");
    return -1;
  }
  if (pid > 0)
    _exit(0);                    // grandparent exits

  setsid();
  signal(SIGHUP, SIG_IGN);

  pid = fork();
  if (pid < 0) {
    printErr("fork #2 failed: ");
    exit(1);
  }
  if (pid > 0)
    _exit(0);                    // parent exits

  if (chdir("/") < 0)
    printErr("chdir to root failed: ");
  umask(0);

  int fd = (int) sysconf(_SC_OPEN_MAX);
  while (--fd > STDERR_FILENO) // leave 0,1,2 alone for now
    close(fd);

  if (pidPath) {
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "%lu\n", (unsigned long) getpid());
    fd = open(pidPath, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd >= 0) {
      fullWrite(fd, buf, len);
      close(fd);
    }
  }

  return 0;
}


static int
redirect(const char *stdIn, const char *stdOut, const char *stdErr)
{
  int fd;

  if (stdIn) {
    fd = open(stdIn, O_RDONLY);
    if (fd < 0)
      return fd;
    dup2(fd, STDIN_FILENO);
    close(fd);
  }
  if (stdOut) {
    fd = open(stdOut, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0666);
    if (fd < 0)
      return fd;
    dup2(fd, STDOUT_FILENO);
    close(fd);
  }
  if (stdErr) {
    fd = open(stdErr, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0666);
    if (fd < 0)
      return fd;
    dup2(fd, STDERR_FILENO);
    close(fd);
  }

  return 0;
}


struct lgPatT {
  lgPatT   *next;
  regex_t   reg;

  lgPatT(const char *regex, int flags) : next(NULL) {
    if (regcomp(&reg, regex, flags) != 0) throw "failed to compile regex"; }
  ~lgPatT() { regfree(&reg); }
  bool Match(const char *str) { return (regexec(&reg, str, 0, NULL, 0) == 0); }
};


int
main(int argc, char **argv)
{
  int fd = -1;
  const char *target = "/var/log/syslog";
  const char *pidfn = "/tmp/loggle.pid";
  const char *logfn = "/tmp/loggle.out";
  const char *email = NULL;
  const char *subject = NULL;
  lgPatT     *pat;
  lgPatT     *patHead = NULL;
  lgPatT     *patTail = NULL;
  int batch = 100;
  int interval = 300;
  int hitThres = 1;
  int lineClear = 0x7fffffff;
  int flags = REG_EXTENDED | REG_NOSUB | REG_NEWLINE;
  bool fg = false;
  char buf[2048];

  try {
    char **argp;
    char **argend = argv + argc;
    for (argp = argv + 1; argp < argend; ++argp) {
      if (**argp != '-')
        break;
      char *arg = *argp;
      if (!strcmp(arg, "-i"))
        flags |= REG_ICASE;
      else if (!strcmp(arg, "-fg"))
        fg = true;
      else if (!strcmp(arg, "-t")) {
        if (++argp >= argend)
          throw "missing -t time";
        interval = atoi(*argp);
      }
      else if (!strcmp(arg, "-b")) {
        if (++argp >= argend)
          throw "missing -b count";
        batch = atoi(*argp);
      }
      else if (!strcmp(arg, "-h")) {
        if (++argp >= argend)
          throw "missing -h count";
        hitThres = atoi(*argp);
      }
      else if (!strcmp(arg, "-c")) {
        if (++argp >= argend)
          throw "missing -c count";
        lineClear = atoi(*argp);
      }
      else if (!strcmp(arg, "-f")) {
        if (++argp >= argend)
          throw "missing -f file";
        target = *argp;
      }
      else if (!strcmp(arg, "-e")) {
        if (++argp >= argend)
          throw "missing -e regex";
        pat = new lgPatT(*argp, flags);
        if (patTail)
          patTail->next = pat;
        else
          patHead = pat;
        patTail = pat;
      }
      else if (!strcmp(arg, "-s")) {
        if (++argp >= argend)
          throw "missing -s subject";
        subject = *argp;
      }
      else if (!strcmp(arg, "-m")) {
        if (++argp >= argend)
          throw "missing -m mail address";
        email = *argp;
      }
      else
        throw "unknown option";
    }
    if (argp < argend)
      throw "too many arguments";
    if (!patHead)
      throw "no regular expression specified";
    if (!email)
      throw "no email address specified";
    if (interval < 1)
      throw "invalid time interval";
  }
  catch (const char *msg) {
    usage();
    fprintf(stderr, "%s\n", msg);
    return 1;
  }

  if (!fg) {
    if (daemonize(pidfn) < 0)
      return 1;
    redirect("/dev/null", logfn, logfn);
  }

  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);

  LgMsgListT mlist;
  mlist.Init(batch, interval, email, subject);
  pthread_t thr;
  pthread_create(&thr, NULL, &runner, &mlist);

  signal(SIGINT,  handleSig);
  signal(SIGQUIT, handleSig);
  signal(SIGTERM, handleSig);

  LgTailT lgt;
  lgt.Init(target);

  int lineCnt = 0;
  int hitCnt = 0;
  while (!gQuitReq) {
    lgt.ReadLine(buf, sizeof(buf));
    if (++lineCnt >= lineClear) {
      lineCnt = 0;
      hitCnt = 0;
    }
    for (pat = patHead; pat; pat = pat->next)
      if (pat->Match(buf)) {
	lineCnt = 0;
        ++hitCnt;
        break;
      }
    if (hitCnt >= hitThres) {
      mlist.Append(buf);
      hitCnt = 0;
    }
  }

  lgPatT *q;
  for (pat = patHead; pat; pat = q) {
    q = pat->next;
    delete pat;
  }
  pthread_join(thr, NULL);
  return 0;
}
