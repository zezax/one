// capped vector header
// avoids extra power-of-two overhead when max size is known
// limit passed to safeRef() so this can be default-constructed
// growth by 1.5x tests no slower than doubling and saves memory

#pragma once

#include <cstdlib>
#include <cstring>

#include <new>
#include <system_error>

namespace zezax::red {

#pragma GCC diagnostic ignored "-Wclass-memaccess"

// only for bitwise-copyable elements
template <class T>
class CappedVec {
public:
  CappedVec() : ary_(nullptr), size_(0), alloc_(0) {}

  CappedVec(const CappedVec &other) : ary_(nullptr), size_(0), alloc_(0) {
    if (other.empty())
      return;
    ary_ = static_cast<T *> (std::malloc(other.size_ * sizeof(T)));
    if (!ary_)
      throw std::bad_alloc();
    alloc_ = other.size_;
    size_ = other.size_;
    for (size_t ii = 0; ii < size_; ++ii)
      new (ary_ + ii) T(other.ary_[ii]); // placement new copy constructor
  }

  CappedVec(CappedVec &&other)
    : ary_(std::exchange(other.ary_, nullptr)),
      size_(std::exchange(other.size_, 0)),
      alloc_(std::exchange(other.alloc_, 0)) {}

  ~CappedVec() { cleanup(); }

  CappedVec &operator=(const CappedVec &rhs);

  CappedVec &operator=(CappedVec &&rhs) {
    cleanup();
    ary_ = std::exchange(rhs.ary_, nullptr);
    size_ = std::exchange(rhs.size_, 0);
    alloc_ = std::exchange(rhs.alloc_, 0);
    return *this;
  }

  T &operator[](size_t idx) const { return ary_[idx]; }

  size_t size() const { return size_; }
  size_t capacity() const { return alloc_; }
  bool empty() const { return (size_ == 0); }

  T *begin() const { return ary_; }
  T *end() const { return ary_ + size_; }

  T &safeRef(size_t idx, size_t limit) {
    if (idx >= alloc_) {
      size_t num = newSize(idx, limit);
      T *big = static_cast<T *>(std::realloc(ary_, num * sizeof(T)));
      if (!big)
        throw std::system_error(errno, std::generic_category(),
                                "realloc failed to grow capped vec");
      ary_ = big;
      alloc_ = num;
    }

    for (; idx >= size_; ++size_)
      new (ary_ + size_) T(); // placement new

    return ary_[idx];
  }

private:
  void cleanup() {
    for (size_t ii = 0; ii < size_; ++ii)
      ary_[ii].~T(); // explicit destructor
    std::free(ary_);
  }

  size_t newSize(size_t idx, size_t limit) {
    if (idx >= limit)
      throw std::bad_alloc();
    size_t need = idx + 1;
    size_t mult = size_ + (size_ / 2); // 1.5x
    if (mult > need)
      need = mult;
    return (need < limit) ? need : limit; // this is the key part
  }

  T      *ary_;
  size_t  size_;
  size_t  alloc_;
};


template <class T>
CappedVec<T> &CappedVec<T>::operator=(const CappedVec<T> &rhs) {
  if (rhs.empty()) {
    cleanup();
    ary_ = nullptr;
    size_ = 0;
    alloc_ = 0;
    return *this;
  }

  // for constructed elements in common, simple copy
  size_t lower = std::min(size_, rhs.size_);
  for (size_t ii = 0; ii < lower; ++ii)
    ary_[ii] = rhs.ary_[ii];

  // for allocated but not constructed, if in range, copy construct
  size_t limit = std::min(alloc_, rhs.size_);
  for (size_t ii = size_; ii < limit; ++ii)
    new (ary_ + ii) T(rhs.ary_[ii]); // placement new

  // for constructed elements not in new range, destruct
  for (size_t ii = rhs.size_; ii < size_; ++ii)
    ary_[ii].~T(); // explicit destructor

  size_ = limit; // valid prior to any extension

  // now deal with extending the array...
  if (alloc_ < rhs.size_) {
    T *big = static_cast<T *>(std::realloc(ary_, rhs.size_ * sizeof(T)));
    if (!big)
      throw std::bad_alloc();
    for (size_t ii = alloc_; ii < rhs.size_; ++ii)
      new (big + ii) T(rhs.ary_[ii]); // placement new copy constructor
    ary_ = big;
    size_ = rhs.size_;
    alloc_ = rhs.size_;
  }

  return *this;
}

} // namespace zezax::red
