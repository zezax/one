// bit set class header
// implements set of unsigned integers as variable-size bitmask

#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

#include "Fnv.h"

namespace zezax::red {

// forward declaration of the main attraction, below
template <class Index, class Tag, class Word> class BitSet;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS DEFINITIONS
//
///////////////////////////////////////////////////////////////////////////////

struct DefaultTag {};


template <class Index, class Tag, class Word>
class BitSetIter {
public:
  static constexpr Index wordBits_  = std::numeric_limits<Word>::digits;
  static constexpr Index indexFFFF_ = static_cast<Index>(0) - 1;
  static constexpr Word  one_       = static_cast<Word>(1);

  BitSetIter() {}
  BitSetIter(const BitSetIter &other)
  : ptr_(other.ptr_), limit_(other.limit_), bit_(other.bit_) {}

  BitSetIter &operator=(const BitSetIter &rhs) {
    ptr_ = rhs.ptr_;
    limit_ = rhs.limit_;
    bit_ = rhs.bit_;
    return *this;
  }

  Index operator*() const { return bit_; }

  bool operator==(const BitSetIter &rhs) const {
    return ((ptr_ == rhs.ptr_) && (bit_ == rhs.bit_));
  }

  bool operator!=(const BitSetIter &rhs) const {
    return ((ptr_ != rhs.ptr_) || (bit_ != rhs.bit_));
  }

  BitSetIter &operator++();

private:
  BitSetIter(std::nullptr_t)
  : ptr_(nullptr), limit_(nullptr), bit_(indexFFFF_) {}

  BitSetIter(const Word *ptr, Index words)
  : ptr_(ptr), limit_(ptr + words), bit_(0) {
    --bit_;
    ++*this;
  }

  const Word *ptr_;
  const Word *limit_;
  Index       bit_;

  friend class BitSet<Index, Tag, Word>;
};

///////////////////////////////////////////////////////////////////////////////

// depending on application uint16_t or uint32_t may be more efficient
template <class Index, class Tag = DefaultTag, class Word = uint64_t>
class BitSet {
public:
  static_assert(std::is_integral_v<Index>);
  static_assert(std::is_unsigned_v<Word>);

  static constexpr Index wordBits_  = std::numeric_limits<Word>::digits;
  static constexpr Word  wordFFFF_  = static_cast<Word>(0) - 1;
  static constexpr Word  one_       = static_cast<Word>(1);

  typedef BitSetIter<Index, Tag, Word> Iter;

  BitSet() = default;
  explicit BitSet(Index idx) { set(idx); }
  BitSet(Index first, Index last) { setSpan(first, last); }

  bool operator<(const BitSet& rhs) const;
  bool operator==(const BitSet& rhs) const;
  bool operator!=(const BitSet& rhs) const { return !operator==(rhs); }

  Index size() const { return wordBits_ * rawSize(); } // rounded up

  void resize(Index bits); // will round up

  void set(Index idx) {
    ensure(idx);
    vec_[idx / wordBits_] |= one_ << (idx % wordBits_);
  }

  void setSpan(Index first, Index last);

  void clear(Index idx) {
    ensure(idx);
    vec_[idx / wordBits_] &= ~(one_ << (idx % wordBits_));
  }

  bool get(Index idx) const {
    if (size() > idx)
      return ((vec_[idx / wordBits_] & (one_ << (idx % wordBits_))) != 0);
    return false;
  }

  bool testAndSet(Index idx) {
    ensure(idx);
    Word &word = vec_[idx / wordBits_];
    Word mask = one_ << (idx % wordBits_);
    bool rv = ((word & mask) != 0);
    word |= mask;
    return rv;
  }

  void clearAll() { vec_.clear(); }

  void setAll() {
    for (Word &ref : vec_)
      ref = wordFFFF_;
  }

  void flipAll() {
    for (Word &ref : vec_)
      ref ^= wordFFFF_;
  }

  void intersectWith(const BitSet &other);
  void unionWith(const BitSet &other);
  void xorWith(const BitSet &other);
  void subtract(const BitSet &other);

  bool hasIntersection(const BitSet &other) const;
  bool contains(const BitSet &other) const;

  Index population() const;
  size_t hash() const;

  Iter begin() const { return Iter(vec_.data(), rawSize()); }
  Iter end() const { return endIter_; }

private:
  void ensure(Index bit) {
    Index word = bit / wordBits_;
    while (vec_.size() <= static_cast<size_t>(word))
      vec_.push_back(0);
  }

  static Word calcMask(Index nbits) {
    if (nbits >= BitSet::wordBits_) {
      Word x = 0;
      return x - 1;
    }
    return (one_ << nbits) - 1;
  }

  Index rawSize() const { return static_cast<Index>(vec_.size()); }

  std::vector<Word> vec_;

  static const Iter endIter_;

  friend Iter;
};

///////////////////////////////////////////////////////////////////////////////

template <class T> inline int trailZeros(T x) { return __builtin_ctz(x); }

template <> inline int trailZeros(unsigned long x) { return __builtin_ctzl(x); }

template <> inline int trailZeros(unsigned long long x) {
  return __builtin_ctzll(x);
}


template <class T> inline int popCount(T x) { return __builtin_popcount(x); }

template <> inline int popCount(unsigned long x) {
  return __builtin_popcountl(x);
}

template <> inline int popCount(unsigned long long x) {
  return __builtin_popcountll(x);
}

///////////////////////////////////////////////////////////////////////////////
//
// MEMBER IMPLEMENTATIONS
//
///////////////////////////////////////////////////////////////////////////////

// this was a performance-critical function
template <class Index, class Tag, class Word>
BitSetIter<Index, Tag, Word> &BitSetIter<Index, Tag, Word>::operator++() {
  Index bit = bit_ + 1;
  Index word = bit / wordBits_;
  Index shift = bit % wordBits_;
  const Word *p = ptr_ + word;
  while (p < limit_) {
    Word val = *p;
    if (shift == 0)
      if (val == 0)
        ++p;
      else {
        bit_ = (static_cast<Index>(p - ptr_) * wordBits_) + trailZeros(val);
        return *this;
      }
    else if ((val & (one_ << shift)) == 0) {
      if (++shift >= wordBits_) {
        shift = 0;
        ++p;
      }
    }
    else {
      bit_ = (static_cast<Index>(p - ptr_) * wordBits_) + shift;
      return *this;
    }
  }
  ptr_ = nullptr;
  bit_ = indexFFFF_;
  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Index, class Tag, class Word>
const BitSetIter<Index, Tag, Word> BitSet<Index, Tag, Word>::endIter_(nullptr);


template <class Index, class Tag, class Word>
bool BitSet<Index, Tag, Word>::operator<(
    const BitSet<Index, Tag, Word>& rhs) const {
  Index mySize = rawSize();
  Index rhsSize = rhs.rawSize();
  Index limit = std::min(mySize, rhsSize);
  Index ii;
  for (ii = 0; ii < limit; ++ii) {
    Word my = vec_[ii];
    Word rh = rhs.vec_[ii];
    if (my < rh)
      return true;
    if (my > rh)
      return false;
  }
  if (mySize > rhsSize) {
    for (; ii < mySize; ++ii)
      if (vec_[ii])
        return false;
  }
  else
    for (; ii < rhsSize; ++ii)
      if (rhs.vec_[ii])
        return true;
  return false;
}


template <class Index, class Tag, class Word>
bool BitSet<Index, Tag, Word>::operator==(
    const BitSet<Index, Tag, Word>& rhs) const {
  Index mySize = rawSize();
  Index rhsSize = rhs.rawSize();
  Index limit = std::min(mySize, rhsSize);
  Index ii;
  for (ii = 0; ii < limit; ++ii)
    if (vec_[ii] != rhs.vec_[ii])
      return false;
  if (mySize > rhsSize) {
    for (; ii < mySize; ++ii)
      if (vec_[ii])
        return false;
  }
  else
    for (; ii < rhsSize; ++ii)
      if (rhs.vec_[ii])
        return false;
  return true;
}


template <class Index, class Tag, class Word>
void BitSet<Index, Tag, Word>::resize(Index bits) {
  Index old = rawSize();
  Index words = (bits + wordBits_ - 1) / wordBits_;
  vec_.resize(words);
  if (words < old) {
    Index roundBits = wordBits_ * words;
    if (roundBits > bits) {
      Index extra = roundBits - bits;
      Word mask = calcMask(extra);
      mask <<= wordBits_ - extra;
      vec_[words - 1] &= ~mask;
    }
  }
}


template <class Index, class Tag, class Word>
void BitSet<Index, Tag, Word>::setSpan(Index first, Index last) {
  ensure(last);
  Index firstWord = first / wordBits_;
  Index lastWord = last / wordBits_;
  Index firstBit = first % wordBits_;
  if (firstWord == lastWord) {
    Index nbits = last - first + 1;
    Word mask = calcMask(nbits);
    mask <<= firstBit;
    vec_[firstWord] |= mask;
  }
  else if (firstWord < lastWord) {
    Index nbits = wordBits_ - firstBit;
    Word mask = calcMask(nbits);
    mask <<= firstBit;
    vec_[firstWord] |= mask;
    for (Index ii = firstWord + 1; ii < lastWord; ++ii)
      vec_[ii] = wordFFFF_;
    Index lastBit = last % wordBits_;
    mask = calcMask(lastBit + 1);
    vec_[lastWord] |= mask;
  }
}


template <class Index, class Tag, class Word>
void BitSet<Index, Tag, Word>::intersectWith(
    const BitSet<Index, Tag, Word> &other) {
  Index mySize = rawSize();
  Index otherSize = other.rawSize();
  if (mySize > otherSize) {
    mySize = otherSize;
    vec_.resize(mySize);
  }
  for (Index ii = 0; ii < mySize; ++ii)
    vec_[ii] &= other.vec_[ii];
}


template <class Index, class Tag, class Word>
void BitSet<Index, Tag, Word>::unionWith(
    const BitSet<Index, Tag, Word> &other) {
  Index mySize = rawSize();
  Index otherSize = other.rawSize();
  if (mySize < otherSize)
    vec_.resize(otherSize);
  for (Index ii = 0; ii < otherSize; ++ii)
    vec_[ii] |= other.vec_[ii];
}


template <class Index, class Tag, class Word>
void BitSet<Index, Tag, Word>::xorWith(const BitSet<Index, Tag, Word> &other) {
  Index mySize = rawSize();
  Index otherSize = other.rawSize();
  if (mySize < otherSize)
    vec_.resize(otherSize);
  for (Index ii = 0; ii < otherSize; ++ii)
    vec_[ii] ^= other.vec_[ii];
}


template <class Index, class Tag, class Word>
void BitSet<Index, Tag, Word>::subtract(const BitSet<Index, Tag, Word> &other) {
  Index limit = std::min(rawSize(), other.rawSize());
  for (Index ii = 0; ii < limit; ++ii)
    vec_[ii] &= ~other.vec_[ii];
}


// this is a performance-critical function
template <class Index, class Tag, class Word>
bool BitSet<Index, Tag, Word>::hasIntersection(
    const BitSet<Index, Tag, Word> &other) const {
  Index limit = std::min(rawSize(), other.rawSize());
  for (Index ii = 0; ii < limit; ++ii)
    if (vec_[ii] & other.vec_[ii])
      return true;
  return false;
}


template <class Index, class Tag, class Word>
bool BitSet<Index, Tag, Word>::contains(
    const BitSet<Index, Tag, Word> &other) const {
  Index mySize = rawSize();
  Index otherSize = other.rawSize();
  Index limit = std::min(mySize, otherSize);
  Index ii;
  for (ii = 0; ii < limit; ++ii)
    if (~vec_[ii] & other.vec_[ii])
      return false;
  for (; ii < otherSize; ++ii)
    if (other.vec_[ii])
      return false;
  return true;
}


template <class Index, class Tag, class Word>
Index BitSet<Index, Tag, Word>::population() const {
  Index rv = 0;
  for (Word x : vec_)
    rv += popCount(x);
  return rv;
}


template <class Index, class Tag, class Word>
size_t BitSet<Index, Tag, Word>::hash() const {
  size_t limit = 0;
  size_t n = vec_.size();
  for (size_t ii = 0; ii < n; ++ii)
    if (vec_[ii])
      limit = ii + 1;
  return fnv1a<size_t>(vec_.data(), limit * sizeof(Word));
}

} // namespace zezax::red
