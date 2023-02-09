// small utility functions

#pragma once

#include <map>
#include <string>
#include <vector>

#include "Types.h"

namespace zezax::red {

char fromHexDigit(Byte x);

void writeStringToFile(const std::string &str, const char *path);
std::string readFileToString(const char *path);

size_t bytesUsed();


template <class T>
T &safeRef(std::vector<T> &vec, size_t idx) {
  if (vec.size() <= idx)
    vec.resize(idx + 1);
  return vec[idx];
}


template <class T>
bool contains(const std::vector<T> &vec, const T &want) {
  for (const T &elem : vec)
    if (elem == want)
      return true;
  return false;
}


template <class K, class T>
bool contains(const std::map<K, T> &map, const T &want) {
  for (const auto &[key, val] : map)
    if (val == want)
      return true;
  return false;
}

} // namespace zezax::red
