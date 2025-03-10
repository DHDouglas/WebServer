#pragma once

#include <thread>
#include <functional>
#include <string>
#include <atomic>

#include "count_down_latch.h"

namespace CurrentThread {

extern thread_local int t_cached_tid; 
extern thread_local char t_tid_string[32];
extern thread_local int t_tid_string_len; 
extern const char* t_thread_name; 

void cachedTid(); 
bool isMainThread(); 

inline int getTid() {
    // 编译器提供的分支预测优化函数, 优化执行效率(t_cached_tid==0的情况仅在线程刚创建时发生)
    if (__builtin_expect(t_cached_tid == 0, 0)) {
        cachedTid(); 
    }
    return t_cached_tid;  
}

inline const char* tidString() { return t_tid_string; }

inline int tidStringLength() { return t_tid_string_len; }

inline const char* name() { return t_thread_name; }

}  // namespace CurrentThread


class Thread {
public:
    using ThreadFunc = std::function<void()>; 

    explicit Thread(ThreadFunc func, const std::string& name = ""); 
    ~Thread(); 

    // movable
    // Thread(Thread&& rhs) noexcept; 
    // Thread& operator=(Thread&& rhs) noexcept; 

    void start();
    void join(); 

    bool isStarted() { return started_; }
    pid_t tid() const { return tid_; }
    const std::string& name() const { return name_; }

    static int getNumCreated() { return num_created_.load(); } 

private:
    void threadFunc(); 
    bool started_; 
    std::thread thread_; 
    pid_t tid_;   // 用Linux的gettid(), 而非std::this_thread::get_tid(). 
    ThreadFunc func_; 
    std::string name_;
    CountDownLatch latch_; 

    static std::atomic<int> num_created_; 
};