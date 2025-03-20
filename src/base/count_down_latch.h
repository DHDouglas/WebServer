#pragma once

#include <mutex>
#include <condition_variable>
#include <cassert>

class CountDownLatch {
public:
    explicit CountDownLatch(int count) : count_(count) {
        assert(count > 0); 
    }

    void wait(); 
    void countDown();  // 计数 -1
    int getCount() const; 

private:
    mutable std::mutex mtx_; 
    std::condition_variable cond_; 
    int count_; 
};


inline void CountDownLatch::wait() {
    std::unique_lock<std::mutex> lock(mtx_); 
    cond_.wait(lock, [this]{ return count_ <= 0; }); 
}

inline void CountDownLatch::countDown() {
    std::lock_guard<std::mutex> lock(mtx_); 
    --count_;
    if (count_ == 0) {
        cond_.notify_all();   // 唤醒所有等待线程
    } 
}

inline int CountDownLatch::getCount() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return count_;
}