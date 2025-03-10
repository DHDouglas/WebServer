#pragma once 

#include <mutex>
#include <atomic>
#include <condition_variable>

#include "thread.h"

class EventLoop;


class EventLoopThread {
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>; 

public:
    EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(), const std::string& name = ""); 
    ~EventLoopThread(); 
    EventLoop* startLoop(); 

private:
    void threadFunc(); 

private: 
    EventLoop* loop_;               // GUARDED_BY(mtx_)
    Thread thread_; 
    std::atomic<bool> exiting_;  
    std::mutex mtx_;  
    std::condition_variable cond_;  // GUARDED_BY(mtx_)
    ThreadInitCallback init_callback_; 
};