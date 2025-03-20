#pragma once

#include <functional>
#include <utility>
#include <set>
#include <memory>
#include <atomic>

#include "channel.h"
#include "timestamp.h"
 
using TimerCallback = std::function<void()>; 
using Duration = Timestamp::Duration; 

class EventLoop; 

class Timer {
public:
    Timer(TimerCallback cb, Timestamp exp, Duration interval) 
        : callback_(cb), expiration_(exp), interval_(interval) {}
    // ~Timer();

    void run() const {  callback_();  }
    void restart(const Timestamp& now) {
        if (repeatable()) {
            expiration_ = now + interval_;
        } else {
            expiration_ = Timestamp::invalid();
        }
    }

    Timestamp getExpiration() const {  return expiration_; }
    bool repeatable() { return interval_ != Duration(0); }

private:
    TimerCallback callback_;   // 定时器回调任务
    Timestamp expiration_;     // 过期时间
    Duration interval_;        // 回调间隔
};


class TimerManager {
private:
    using Entry = std::pair<Timestamp, std::shared_ptr<Timer>>;
    using TimerList = std::set<Entry>; 
    using ActiveTimerSet = std::set<std::shared_ptr<Timer>>; 

public: 
    TimerManager(EventLoop* loop);
    ~TimerManager();

    std::weak_ptr<Timer> addTimer(TimerCallback cb, Timestamp when, Duration interval); 
    void removeTimer(const std::weak_ptr<Timer>& wk_ptr_timer); 

private:
    void addTimerInLoop(const std::shared_ptr<Timer>& ptr_timer); 
    bool insert(const std::shared_ptr<Timer>& ptr_timer);  // 插入timer, 返回是否需要更新timerfd
    void removeTimerInLoop(const std::weak_ptr<Timer>& wk_ptr_timer);  
    void handleExpiredTimer();    // timerfd触发事件时, 处理到期的定时器事件
    void checkUpdateTimerfd();    // 检查是否需要更新timerfd的到期时间.
    int createTimerfd();

private:
    EventLoop* loop_;
    int timerfd_;
    Channel timerfd_channel_; 
    TimerList timers_; 
    
    ActiveTimerSet  canceling_timers_;   // 暂存需要被撤销的Timer. 
    std::atomic<bool> calling_expired_timers_;  
};