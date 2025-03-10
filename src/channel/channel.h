#pragma once

#include <sys/epoll.h>
#include <functional>
#include <memory>

#include "timestamp.h"

class EventLoop;

class Channel {
public:
    // 标记channel在Epoller中的注册状态: 未持有/已注册监听/已持有但监听
    enum class StateInEpoll { NOT_EXIST = 0, LISTENING, DETACHED };
    using Callback = std::function<void()>; 
    using ReadCallback = std::function<void(Timestamp)>;

public:
    Channel(EventLoop* loop, int fd);
    ~Channel(); 
    void setRevents(int evt); 
    void handleEvents(Timestamp receive_time);
    int getFd() const; 
    int getEvents() const; 
    EventLoop* getOwnerLoop() const;
    StateInEpoll getState();
    void setState(StateInEpoll state); 
    void removeFromEpoller(); 
    void enableReading();
    void disableReading();
    void enableWriting();
    void disableWriting();
    void disableAll(); 
    bool isWriting() const;
    bool isReading() const; 
    bool isNoneEvent() const; 

    void setReadCallback(ReadCallback cb);
    void setWriteCallback(Callback cb); 
    void setErrorCallback(Callback cb); 
    void setCloseCallback(Callback cb); 

    // 延长由shared_ptr或unique_ptr管理的对象的生命周期, 防止其Channel调用handleEvent期间析构.
    // 主要针对Channel对象本身或其owner对象
    void tie(const std::shared_ptr<void>&); 

private:
    void updateInEpoller();    // add or mod
    void handleEventWithGuard(Timestamp receive_time);
    // for debug
    static std::string eventsToString(int fd, int ev); 
    std::string reventsToString() const;
    std::string eventsToString() const;

private:
    static const int kNoneEvent = 0;
    static const int kReadEvent = EPOLLIN | EPOLLPRI;
    static const int kWriteEvent = EPOLLOUT; 

    ReadCallback readCallback_; 
    Callback writeCallback_; 
    Callback errorCallback_; 
    Callback closeCallback_;  

    EventLoop* loop_;     // 每个channel只属于1个eventloop(IO线程)
    const int fd_;        // 始终只关联于一个fd.
    int events_;          // bit pattern: 监听事件集
    int revents_;         // bit pattern: 当前活跃事件集
    bool event_handling;  
    StateInEpoll state_;  // 该channel在epoller中的注册状态. 

    bool tied_;
    std::weak_ptr<void> tie_; 
};