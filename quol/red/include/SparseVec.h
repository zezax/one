/* SparseVec.h - sparse vector header

   SparseVec acts mostly like a vector, providing acces by index,
   but saves memory by storing elements sparsely.  This comes at the
   cost of extra processing.  Lookups are O(log N) and storing new
   values can be O(N).

   In some ways, SparseVec acts like a map.  For instance, size()
   returns the number of valid elements, not the contiguous size.
   Reference to a specific index causes that element to spring into
   existence.  Iteration is over the valid SparseRec structures.

   Usage is like:

   SparseVec<std::string> vec;
   vec[69] = "bar";
   vec[13] = "foo";
   for (const auto &[idx, str] : vec)
     std::cout << idx << '=' << str << std::endl;
 */

#pragma once

#include <vector>

namespace zezax::red {

template <class T>
struct SparseRec {
  size_t idx_;
  T      val_;
};


template <class T>
bool operator==(const SparseRec<T> &aa, const SparseRec<T> &bb) {
  return ((aa.idx_ == bb.idx_) && (aa.val_ == bb.val_));
}


template <class T>
class SparseVec {
public:
  typedef std::vector<SparseRec<T>> Vec;
  typedef typename Vec::iterator Iterator;
  typedef typename Vec::const_iterator CIterator;

  T &operator[](size_t idx);

  bool operator==(const SparseVec &other) const { return (vec_ == other.vec_); }

  size_t size() const { return vec_.size(); }

  Iterator begin() { return vec_.begin(); }
  Iterator end()   { return vec_.end(); }
  CIterator begin() const { return vec_.begin(); }
  CIterator end()   const { return vec_.end(); }

private:
  Vec vec_;
};

///////////////////////////////////////////////////////////////////////////////

template <class T>
T &SparseVec<T>::operator[](size_t seek) {
  size_t arySiz = vec_.size();
  size_t low = 0;
  size_t mid = 0;
  size_t high = arySiz; // one past last index
  while (low < high) {
    mid = (low + high) / 2; // can't overflow because sizeof(SparseRec) > 1
    size_t midVal = vec_[mid].idx_;
    if (seek < midVal)
      high = mid;
    else if (seek > midVal)
      low = mid + 1;
    else
      return vec_[mid].val_;
  }

  SparseRec<T> rec;
  rec.idx_ = seek;

  if (high == arySiz)
    vec_.emplace_back(std::move(rec));
  else {
    size_t pos = arySiz - 1;
    vec_.emplace_back(std::move(vec_[pos])); // extends the array
    for (; pos > high; --pos)
      vec_[pos] = std::move(vec_[pos - 1]);
    vec_[high] = std::move(rec);
  }

  return vec_[high].val_;
}

} // namespace zezax::red
