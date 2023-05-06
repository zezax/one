// log.h - logging header

#pragma once

#include <unistd.h>
#include <syslog.h>

#include <cstring>
#include <string>
#include <string_view>

/*
  Look below.  The functions you want are probably:

  logOut() - lot to stdout
  logErr() - log to stderr
  logSys() - log to syslog
 */

namespace zezax::sensorrd {

// lengths

inline size_t length(char) { return 1; }
inline size_t length(const std::string &x) { return x.size(); }
inline size_t length(std::string_view x) { return x.size(); }
inline size_t length(const char *x) { return strlen(x); }
inline size_t length(double x) { return snprintf(nullptr, 0, "%g", x); }

template <class T> size_t logTen(T x) { size_t rv = 0;
  do { x /= 10; ++rv; } while (x);
  return rv;
}

inline size_t length(unsigned x) { return logTen(x); }
inline size_t length(unsigned long x) { return logTen(x); }

inline size_t length(int x) {
  return (x < 0 ? 1 + logTen((unsigned) -x) : logTen((unsigned) x));
}

inline size_t length(long x) {
  return (x < 0 ? 1 + logTen((unsigned long) -x) : logTen((unsigned long) x));
}


// appends

template <class T> void append(std::string &s, const T &x) { s += x; }

template <> void append<int>(std::string &s, const int &x) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%d", x);
  s += buf;
}

template <> void append<unsigned>(std::string &s, const unsigned &x) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%u", x);
  s += buf;
}

template <> void append<long>(std::string &s, const long &x) {
  char buf[64];
  snprintf(buf, sizeof(buf), "%ld", x);
  s += buf;
}

template <> void append<unsigned long>(std::string &s, const unsigned long &x) {
  char buf[64];
  snprintf(buf, sizeof(buf), "%lu", x);
  s += buf;
}

template <> void append<float>(std::string &s, const float &x) {
  char buf[64];
  snprintf(buf, sizeof(buf), "%g", x);
  s += buf;
}

template <> void append<double>(std::string &s, const double &x) {
  char buf[64];
  snprintf(buf, sizeof(buf), "%g", x);
  s += buf;
}


// variadic templates

inline void concat(std::string &) {}

template <class T, class ...Args>
void concat(std::string &s, T x, Args... args) {
  append(s, x);
  concat(s, args...);
}


inline size_t measure() { return 0; }

template <class T, class ...Args>
size_t measure(T x, Args... args) {
  return length(x) + measure(args...);
}


// main entrypoints
template <class ...Args>
void logOut(Args... args) {
  std::string s;
  s.reserve(measure(args...) + 1);
  concat(s, args...);
  s += '\n';
  write(STDOUT_FILENO, s.data(), s.size());
}


template <class ...Args>
void logErr(Args... args) {
  std::string s;
  s.reserve(measure(args...) + 1);
  concat(s, args...);
  s += '\n';
  write(STDERR_FILENO, s.data(), s.size());
}


template <class ...Args>
void logSys(int priority, Args... args) {
  std::string s;
  s.reserve(measure(args...));
  concat(s, args...);
  syslog(priority, "%s", s.c_str());
}

} // namespace zezax::sensorrd
