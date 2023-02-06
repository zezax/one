#include "Util.h"

#include <unistd.h>
#include <fcntl.h>

#ifdef USE_JEMALLOC
# include <jemalloc.h>
#endif

#include <array>
#include <system_error>

#include "Except.h"

namespace zezax::red {

using std::string;

char fromHexDigit(Byte c) {
  if ((c >= '0') && (c <= '9'))
    return (static_cast<char>(c) - '0');
  if ((c >= 'A') && (c <= 'F'))
    return (static_cast<char>(c) - 'A' + 10);
  if ((c >= 'a') && (c <= 'f'))
    return (static_cast<char>(c) - 'a' + 10);
  return -1;
}

void writeStringToFile(const std::string &str, const char *path) {
  if (!path)
    throw RedExceptApi("Write file path is null");

  int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0666);
  if (fd < 0)
    throw std::system_error(errno, std::generic_category(),
                            "Failed to open file for write");

  const char *ptr = str.data();
  size_t nbytes = str.size();
  while (nbytes > 0) {
    ssize_t chunk =
      (nbytes > 0x7fffffff) ? 0x7fffffff : static_cast<ssize_t>(nbytes);
    ssize_t did = write(fd, ptr, chunk);
    if (did < 0) {
      if (errno == EINTR)
        continue;
      throw std::system_error(errno, std::generic_category(),
                              "Failed to write file");
    }
    ptr += did;
    nbytes -= did;
  }
}


string readFileToString(const char *path) {
  if (!path)
    throw RedExceptApi("Read file path is null");

  int fd = open(path, O_RDONLY | O_CLOEXEC);
  if (fd < 0)
    throw std::system_error(errno, std::generic_category(),
                            "Failed to open file for read");

  string str;
  std::array<char, 65536> buf;
  for (;;) {
    ssize_t got = read(fd, buf.data(), buf.size());
    if (got <= 0) {
      if (got < 0) {
        if (errno == EINTR)
          continue;
        throw std::system_error(errno, std::generic_category(),
                                "Failed to read file");
      }
      else
        break;
    }
    str.append(buf.data(), got);
  }

  str.shrink_to_fit();
  return str;
}


#ifdef USE_JEMALLOC

size_t bytesUsed() {
  mallctl("thread.tcache.flush", nullptr, nullptr, nullptr, 0);
  uint64_t epoch = 1;
  size_t elen = sizeof(epoch);
  mallctl("epoch", &epoch, &elen, &epoch, elen);

  size_t val;
  size_t vlen = sizeof(val);
  mallctl("stats.allocated", &val, &vlen, nullptr, 0);
  return val;
}

#else /* USE_JEMALLOC */

size_t bytesUsed() {
  char buf[1024];
  int fd = open("/proc/self/statm", O_RDONLY);
  if (fd < 0)
    return 0;
  size_t rv = 0;
  ssize_t got = read(fd, buf, sizeof(buf) - 1);
  if (got > 0) {
    buf[got] = '\0';
    const char *p = buf;
    for (; *p; ++p)
      if (*p == ' ')
        break;
    rv = 4096 * strtol(p, nullptr, 0); // rss is second field
  }
  close(fd);
  return rv;
}

#endif /* USE_JEMALLOC */

} // namespace zezax::red
