// Fowler Noll Vo hash header

#pragma once

#include <stddef.h>

#include <cstdint>
#include <type_traits>

namespace zezax::red {

template <class T, class = void> struct FnvParams;

template <class T>
struct FnvParams<T, std::enable_if_t<sizeof(T) == 4>> {
  static constexpr T basis_ = 0x811c9dc5;
  static constexpr T prime_ = 0x01000193;
};

template <class T>
struct FnvParams<T, std::enable_if_t<sizeof(T) == 8>> {
  static constexpr T basis_ = 0xcbf29ce484222325;
  static constexpr T prime_ = 0x00000100000001B3;
};


// incremental hashing of additional bytes
template <class T>
T fnv1aInc(T hash, const void *ptr, size_t nbytes) {
  const uint8_t *bytep = reinterpret_cast<const uint8_t *>(ptr);
  const uint8_t *end = bytep + nbytes;
  for (; bytep < end; ++bytep) {
    hash ^= *bytep;
    hash *= FnvParams<T>::prime_;
  }
  return hash;
}


// most common entrypoint
template <class T>
T fnv1a(const void *ptr, size_t nbytes) {
  T hash = FnvParams<T>::basis_;
  return fnv1aInc(hash, ptr, nbytes);
}

} // namespace zezax::red
