#pragma once 

#include <mutex>
#include <condition_variable>
#include <thread>

#include "eventloop.h"


class EventLoopThread {
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>; 
public:
    EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback()); 
    ~EventLoopThread(); 
    EventLoop* startLoop(); 
    void threadFunc(); 

private: 
    EventLoop* loop_;               // @GUARDED_BY(mtx_)
    std::atomic<bool> exiting_;  
    std::thread t; 
    std::mutex mtx_;  
    std::condition_variable cond_;  // @GUARDED_BY(mtx_)
    ThreadInitCallback init_callback_; 
};