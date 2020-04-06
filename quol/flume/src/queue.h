// queue.h

#pragma once

#include <condition_variable>
#include <mutex>
#include <deque>


template <class T>
class QueueT {
public:
  QueueT(size_t maxSize) : maxSize_(maxSize) { }

  void enqueue(const T &item) {
    std::unique_lock<std::mutex> lock(mtx_);
    while (deque_.size() >= maxSize_)
      cond_.wait(lock);
    deque_.emplace_back(item);
    cond_.notify_one();
  }

  T dequeue() {
    std::unique_lock<std::mutex> lock(mtx_);
    while (deque_.empty())
      cond_.wait(lock);
    T rv(std::move(deque_.front()));
    deque_.pop_front();
    cond_.notify_one();
    return rv;
  }

protected:
  std::deque<T>            deque_;
  size_t                   maxSize_;
  std::mutex               mtx_;
  std::condition_variable  cond_;
};
