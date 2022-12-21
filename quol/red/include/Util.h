// small utility functions

#pragma once

#include <vector>

#include "Defs.h"

namespace zezax::red {

char fromHexDigit(Byte x);

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

} // namespace zezax::red
