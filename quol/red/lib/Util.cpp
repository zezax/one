// small utility functions implementation

#include "Util.h"

#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef USE_JEMALLOC
# include <jemalloc.h>
#endif

#include <array>
#include <limits>
#include <system_error>

#include "Except.h"

namespace zezax::red {

using std::generic_category;
using std::numeric_limits;
using std::string;
using std::string_view;
using std::system_error;
using std::vector;

char fromHexDigit(Byte x) {
  if ((x >= '0') && (x <= '9'))
    return (static_cast<char>(x) - '0');
  if ((x >= 'A') && (x <= 'F'))
    return (static_cast<char>(x) - 'A' + 10);
  if ((x >= 'a') && (x <= 'f'))
    return (static_cast<char>(x) - 'a' + 10);
  return -1;
}


void writeStringToFile(string_view str, const char *path) {
  if (!path)
    throw RedExceptApi("write file path is null");

  int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0666);
  if (fd < 0)
    throw system_error(errno, generic_category(),
                       "failed to open file for write");

  constexpr size_t maxChunk = numeric_limits<ssize_t>::max();
  while (!str.empty()) {
    size_t chunk = std::min(maxChunk, str.size());
    ssize_t did = write(fd, str.data(), static_cast<ssize_t>(chunk));
    if (did < 0) {
      if (errno == EINTR)
        continue;
      close(fd);
      throw system_error(errno, generic_category(), "failed to write file");
    }
    str.remove_prefix(did);
  }

  close(fd);
}


string readFileToString(const char *path) {
  if (!path)
    throw RedExceptApi("read file path is null");

  int fd = open(path, O_RDONLY | O_CLOEXEC);
  if (fd < 0)
    throw system_error(errno, generic_category(),
                       "failed to open file for read");

  struct stat sst;
  int res = fstat(fd, &sst);
  if (res < 0)
    throw system_error(errno, generic_category(), "failed to fstat read file");

  string str;
  str.reserve(sst.st_size);
  std::array<char, 1048576> buf;

  for (;;) {
    ssize_t got = read(fd, buf.data(), buf.size());
    if (got <= 0) {
      if (got < 0) {
        if (errno == EINTR)
          continue;
        close(fd);
        throw system_error(errno, generic_category(), "failed to read file");
      }
      break;
    }
    str.append(buf.data(), got);
  }

  close(fd);
  return str;
}


vector<string> sampleLines(const string &buf, size_t n) {
  size_t lines = 0;
  for (char ch : buf)
    if (ch == '\n')
      ++lines;

  size_t ratio = lines / n;
  size_t seq = 0;
  size_t hits = 0;
  vector<string> rv;
  auto start = buf.cbegin();
  for (auto it = buf.cbegin(); it < buf.cend(); ++it)
    if (*it == '\n') {
      if ((++seq % ratio) == 0) {
        rv.emplace_back(start, it);
        if (++hits >= n)
          break;
      }
      start = it + 1;
    }
  return rv;
}


size_t bytesUsed() {

#ifdef USE_JEMALLOC

  mallctl("thread.tcache.flush", nullptr, nullptr, nullptr, 0);
  uint64_t epoch = 1;
  size_t elen = sizeof(epoch);
  mallctl("epoch", &epoch, &elen, &epoch, elen);

  size_t val;
  size_t vlen = sizeof(val);
  mallctl("stats.allocated", &val, &vlen, nullptr, 0);

#else /* USE_JEMALLOC */

  static long pageBytes = sysconf(_SC_PAGESIZE);
  std::array<char, 1024> buf;
  int fd = open("/proc/self/statm", O_RDONLY);
  if (fd < 0)
    return 0;
  size_t val = 0;
  ssize_t got = read(fd, buf.data(), buf.size() - 1);
  if (got > 0) {
    buf[got] = '\0';
    const char *p = buf.data();
    for (; *p; ++p)
      if (*p == ' ')
        break;
    val = pageBytes * strtol(p, nullptr, 0); // rss is second field
  }
  close(fd);

#endif /* USE_JEMALLOC */

  return val;
}

} // namespace zezax::red
