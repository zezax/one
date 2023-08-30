/* Fnv.h - Fowler Noll Vo hash header

   Implementation of the FNV-1a hash as described here:
   http://www.isthe.com/chongo/tech/comp/fnv/

   This is a byte-oriented implementation that hashes memory given
   pointer and length.

   Templates are used in order to produce either 32- or 64-bit
   hash results.

   While there are hash functions that behave better in hashing
   benchmarks such as smhasher, FNV is in the sweet spot of quality
   and efficiency for small inputs.  This, along with its small
   code size, is why FNV is widely used in general-purpose hash
   tables such as in the C implementation of Python.

   Usage is like:

   std::string s = "Rocky Raccoon";
   unit64_t hash = fnv1a(s.data(), s.size());
 */

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
