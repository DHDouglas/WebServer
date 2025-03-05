#pragma once 

#include <atomic>
#include <unistd.h>
#include <cassert>
#include <thread> 
#include <cstdio>
#include <vector>
#include <memory>
#include <mutex>
#include <cstdlib>
#include <sys/eventfd.h>

#include "epoller.h"
#include "timer.h"
#include "thread.h"
#include "logger.h"

class Channel; 

class EventLoop {
public:
    using Functor = std::function<void()>; 

public:
    EventLoop(); 
    ~EventLoop();
    void loop(); 
    void quit(); 
    bool isInLoopThread();
    void assertInLoopThread();
    void abortNotInLoopThread(); 
    void runInLoop(const Functor& cb);         // 可供其他线程调用
    void addToQueueInLoop(const Functor& cb);  // queueInLoop, 加入当前loop的PendingFunctors. 

    bool hasChannel(Channel* channel);
    void updateChannelInEpoller(Channel* channel);
    void modInEpoller(Channel* channel); 
    void removeChannelFromEpoller(Channel* channel); 

    std::weak_ptr<Timer> runAt(const Timestamp& time, const TimerCallback& cb);
    std::weak_ptr<Timer> runAfter(const Duration& delay, const TimerCallback& cb); 
    std::weak_ptr<Timer> runEvery(const Duration& interval, const TimerCallback& cb); 
    void removeTimer(const std::weak_ptr<Timer>& wk_ptr_timer);  

    static thread_local EventLoop* t_loopInThisThread;  // 线程局部存储持续性, 保证每个线程只有一个EventLoop实例.
    static EventLoop* getEventLoopInThisThread() { return t_loopInThisThread; }

private:
    void doPendingFunctors();        

    void wakeup();        // 在addToQueueInLoop()中使用时, 用于唤醒目标线程的EventLoop, 防止阻塞在epoll_wait.
    void handleWakeUp();  
    int createEventfd();  // eventfd used as wakeup_fd_

public:
    using ChannelList = std::vector<Channel*>; 

private:
    std::atomic<bool> looping_;
    std::atomic<bool> quit_; 
    std::atomic<bool> calling_pending_functors_;  
    std::atomic<bool> event_handling_; 
    const pid_t threadId_; 

    std::unique_ptr<Epoller> epoller_; 
    std::unique_ptr<TimerManager> timer_manager_; 

    ChannelList active_channels_;
    Channel* current_active_channel_; 
    
    int wakeup_fd_;   // eventfd for waking up the epoll_wait. 
    // std::unique_ptr<Channel> wakeup_channel_; 
    Channel wakeup_channel_; 
    
    std::mutex mtx_; 
    std::vector<Functor> pending_functors_;  // guarded by mutex_
};