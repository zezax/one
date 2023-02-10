// sparse vector header

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

  T &operator[](size_t idx);

  bool operator==(const SparseVec &other) const { return (vec_ == other.vec_); }

  size_t size() const { return vec_.size(); }

  Vec::iterator begin() { return vec_.begin(); }
  Vec::iterator end() { return vec_.end(); }
  Vec::const_iterator begin() const { return vec_.begin(); }
  Vec::const_iterator end() const { return vec_.end(); }

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
