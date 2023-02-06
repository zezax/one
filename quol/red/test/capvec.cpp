// unit tests for capped vector

#include <gtest/gtest.h>

#include "CappedVec.h"

using namespace zezax::red;

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

private:
  int *ptr_;
};

} // anonymous

TEST(CappedVec, smoke) {
  CappedVec<Elem> vec;
  vec.safeRef(0,  11) = 13;
  EXPECT_EQ(1, vec.size());
  EXPECT_EQ(1, vec.capacity());
  vec.safeRef(5,  11) = 69;
  EXPECT_EQ(6, vec.size());
  EXPECT_EQ(6, vec.capacity());
  vec.safeRef(10, 11) = 666;
  ASSERT_EQ(11, vec.size());
  EXPECT_EQ(11, vec.capacity());
  EXPECT_EQ(13,  vec[0]);
  EXPECT_EQ(0,   vec[1]);
  EXPECT_EQ(0,   vec[2]);
  EXPECT_EQ(0,   vec[3]);
  EXPECT_EQ(0,   vec[4]);
  EXPECT_EQ(69,  vec[5]);
  EXPECT_EQ(0,   vec[6]);
  EXPECT_EQ(0,   vec[7]);
  EXPECT_EQ(0,   vec[8]);
  EXPECT_EQ(0,   vec[9]);
  EXPECT_EQ(666, vec[10]);
  EXPECT_THROW(vec.safeRef(11, 11), std::bad_alloc);

  CappedVec<Elem> v1 = vec;
  ASSERT_EQ(11, v1.size());
  EXPECT_EQ(13,  v1[0]);
  EXPECT_EQ(0,   v1[1]);
  EXPECT_EQ(69,  v1[5]);
  EXPECT_EQ(0,   v1[9]);
  EXPECT_EQ(666, v1[10]);

  CappedVec<Elem> v2(vec);
  ASSERT_EQ(11, v2.size());
  EXPECT_EQ(13,  v2[0]);
  EXPECT_EQ(0,   v2[1]);
  EXPECT_EQ(69,  v2[5]);
  EXPECT_EQ(0,   v2[9]);
  EXPECT_EQ(666, v2[10]);
}


TEST(CappedVec, copy) {
  constexpr size_t limit = 1500;

  CappedVec<Elem> big;
  for (size_t ii = 0; ii < 1000; ++ii)
    big.safeRef(ii, limit) = 6969;
  EXPECT_EQ(1000, big.size());

  CappedVec<Elem> med;
  for (size_t ii = 0; ii < 100; ++ii)
    med.safeRef(ii, limit) = 1313;
  EXPECT_EQ(100, med.size());

  CappedVec<Elem> small;
  for (size_t ii = 0; ii < 10; ++ii)
    small.safeRef(ii, limit) = 420;
  EXPECT_EQ(10, small.size());

  CappedVec<Elem> tiny;
  tiny.safeRef(0, limit) = 69;
  EXPECT_EQ(1, tiny.size());

  CappedVec<Elem> empty;
  EXPECT_EQ(0, empty.size());

  small = med;
  EXPECT_EQ(100, small.size());
  EXPECT_EQ(1313, small[0]);

  CappedVec<Elem> sml(med);
  EXPECT_EQ(100, sml.size());
  EXPECT_EQ(1313, sml[0]);

  big = med;
  EXPECT_EQ(100, big.size());
  EXPECT_EQ(1313, big[0]);

  small = tiny;
  EXPECT_EQ(1, small.size());
  EXPECT_EQ(69, small[0]);

  med = empty;
  EXPECT_EQ(0, med.size());

  tiny = std::move(big);
  EXPECT_EQ(100, tiny.size());
  EXPECT_EQ(1313, tiny[0]);

  empty = tiny;
  EXPECT_EQ(100, empty.size());
  EXPECT_EQ(1313, empty[0]);
}
