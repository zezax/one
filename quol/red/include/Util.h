// small utility functions header

#pragma once

#include <map>
#include <string>
#include <vector>

#include "Types.h"

namespace zezax::red {

char fromHexDigit(Byte x);

void writeStringToFile(const std::string &str, const char *path);
std::string readFileToString(const char *path);
std::vector<std::string> sampleLines(const std::string &buf, size_t n);

size_t bytesUsed(); // generally reports resident set size


template <class T>
T &safeRef(std::vector<T> &vec, size_t idx) {
  if (vec.size() <= idx)
    vec.resize(idx + 1);
  return vec[idx];
}


template <class T>
bool contains(const std::vector<T> &vec, const T &seek) {
  for (const T &elem : vec)
    if (elem == seek)
      return true;
  return false;
}


template <class K, class T>
bool contains(const std::map<K, T> &map, const T &seek) {
  for (const auto &[_, val] : map)
    if (val == seek)
      return true;
  return false;
}


template <class T>
bool contains(const SparseVec<T> &vec, const T &seek) {
  for (const auto &[_, elem] : vec)
    if (elem == seek)
      return true;
  return false;
}

} // namespace zezax::red
