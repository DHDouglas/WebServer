#pragma once

#include <functional>
#include <vector>
#include <list>
#include <memory>

#include "timer.h"
#include "any.h"

class EventLoop;

class TimingWheel {
public:
    struct Entry; 
    using EntryPtr = std::shared_ptr<Entry>;
    using EntryWeakPtr = std::weak_ptr<Entry>;  
    using Buckets = std::vector<std::list<EntryPtr>>; 
    using Callback = std::function<void(Any& data)>; 
    using BaseTimer = std::weak_ptr<Timer>; 

    struct Entry {
        explicit Entry(TimingWheel* wheel, Any data); 
        ~Entry();
        TimingWheel* owner_wheel_;
        Any data_;             // 任意类型数据
        int last_bucket_idx_;  // 记录最后一次插入的槽位
        bool valid;            // 标记是否有效.
    };

public:
    TimingWheel(EventLoop* loop, int idle_seconds, Callback cb);
    EntryWeakPtr insert(Any data);
    void update(const EntryWeakPtr& entry_wkptr);
    void remove(const EntryWeakPtr& entry_wkptr);

private:
    void onTimer(); 
    void insertInLoop(const EntryPtr& ptr);
    void updateInLoop(const EntryWeakPtr& entry_wkptr);
    void removeInLoop(const EntryWeakPtr& entry_wkptr);

private:
    EventLoop* loop_; 
    BaseTimer base_timer_;
    const int idle_seconds_;  // 连接超时定时. 等同于槽数. 
    int begin_idx_;           // 当前时刻指向的槽(需清除)
    int end_idx_;             // 当前时刻末尾的槽(供插入)
    Buckets buckets_;   
    Callback callback_;     
};