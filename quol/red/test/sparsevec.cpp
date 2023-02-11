// sparse vector test

#include <vector>

#include <gtest/gtest.h>

#include "SparseVec.h"

using namespace zezax::red;

using std::vector;

namespace {

// boxed int, makes memory errors more apparent
class Elem {
public:
  Elem() : ptr_(nullptr) { ptr_ = new int(); }
  Elem(int x) : ptr_(nullptr) { ptr_ = new int(x); }
  ~Elem() { delete ptr_; ptr_ = nullptr; }
  Elem(const Elem &other) : ptr_(nullptr) { ptr_ = new int(*other.ptr_); }
  Elem(Elem &&other) = delete;
  Elem &operator=(const Elem &rhs) { *ptr_ = *rhs.ptr_; return *this; }
  Elem &operator=(Elem &&rhs) = delete;
  operator int() const { return *ptr_; }
  Elem &operator=(int x) { *ptr_ = x; return *this; }
  bool operator==(int other) const { return (*ptr_ == other); }
  bool operator==(const Elem &other) const { return (*ptr_ == *other.ptr_); }

private:
  int *ptr_;
};

} // anonymous

TEST(SparseVec, smoke) {
  SparseVec<Elem> vec;
  vec[0] = 13;
  EXPECT_EQ(1, vec.size());
  vec[10] = 66;
  EXPECT_EQ(2, vec.size());
  vec[5] = 69;
  EXPECT_EQ(3, vec.size());
  vec[10] = 666;
  EXPECT_EQ(3, vec.size());
  vec[99] = 999;
  ASSERT_EQ(4, vec.size());
  EXPECT_EQ(13,  vec[0]);
  EXPECT_EQ(69,  vec[5]);
  EXPECT_EQ(666, vec[10]);
  EXPECT_EQ(999, vec[99]);

  SparseVec<Elem> v1 = vec;
  ASSERT_EQ(4, v1.size());
  EXPECT_EQ(13,  v1[0]);
  EXPECT_EQ(69,  v1[5]);
  EXPECT_EQ(666, v1[10]);
  EXPECT_EQ(999, v1[99]);

  SparseVec<Elem> v2(std::move(vec));
  ASSERT_EQ(4, v2.size());
  EXPECT_EQ(13,  v2[0]);
  EXPECT_EQ(69,  v2[5]);
  EXPECT_EQ(666, v2[10]);
  EXPECT_EQ(999, v2[99]);
}


TEST(SparseVec, iter) {
  SparseVec<Elem> vec;
  vec[69] = 0;
  vec[12] = 1;
  vec[15] = 2;
  vec[5] = 3;
  vec[99] = 4;
  vec[77] = 5;
  vec[13] = 6;
  vec[10] = 7;
  EXPECT_EQ(8, vec.size());

  SparseVec<Elem> copy;
  for (const auto &[idx, val] : vec)
    copy[idx] = val;
  EXPECT_EQ(vec, copy);

  vector<size_t> keys;
  vector<int> vals;
  for (const auto &[idx, val] : vec) {
    keys.push_back(idx);
    vals.push_back(val);
  }
  ASSERT_EQ(8, keys.size());
  EXPECT_EQ(5, keys[0]);
  EXPECT_EQ(10, keys[1]);
  EXPECT_EQ(12, keys[2]);
  EXPECT_EQ(13, keys[3]);
  EXPECT_EQ(15, keys[4]);
  EXPECT_EQ(69, keys[5]);
  EXPECT_EQ(77, keys[6]);
  EXPECT_EQ(99, keys[7]);
  ASSERT_EQ(8, vals.size());
  EXPECT_EQ(3, vals[0]);
  EXPECT_EQ(7, vals[1]);
  EXPECT_EQ(1, vals[2]);
  EXPECT_EQ(6, vals[3]);
  EXPECT_EQ(2, vals[4]);
  EXPECT_EQ(0, vals[5]);
  EXPECT_EQ(5, vals[6]);
  EXPECT_EQ(4, vals[7]);
}
