#pragma once

#include <unistd.h>
#include <sys/epoll.h>
#include <functional>

class EventLoop;

class Channel {
public:
    // 标记channel在Epoller中的注册状态: 未持有/已注册监听/已持有但监听
    enum class StateInEpoll { NOT_EXIST = 0, LISTENING, DETACHED };

public :
    using Callback = std::function<void()>; 

    Channel(EventLoop* loop, int fd);
    ~Channel(); 
    
    void setRevents(int evt); 
    void handleEvents();
    int getFd() const; 
    int getEvents() const; 
    EventLoop* getOwnerLoop() const;

    StateInEpoll getState();
    void setState(StateInEpoll state); 
    void updateInEpoller();    // add or mod
    void removeFromEpoller(); 

    void enableReading();
    void disableReading();
    void enableWriting();
    void disableWriting();
    void disableAll(); 
    bool isWriting() const;
    bool isReading() const; 
    bool isNoneEvent() const; 

    void setReadCallback(Callback cb);
    void setWriteCallback(Callback cb); 
    void setErrorCallback(Callback cb); 
    void setCloseCallback(Callback cb); 

private:
    static const int kNoneEvent = 0;
    static const int kReadEvent = EPOLLIN | EPOLLPRI;
    static const int kWriteEvent = EPOLLOUT; 

    Callback readCallback_; 
    Callback writeCallback_; 
    Callback errorCallback_; 
    Callback closeCallback_;  

    EventLoop* loop_;   // 每个channel只属于1个eventloop(IO线程)
    const int fd_;      // 始终只关联于一个fd.
    int events_;        // bit pattern: 监听事件集
    int revents_;       // bit pattern: 当前活跃事件集
    bool event_handling;  
    StateInEpoll state_;  // 该channel在epoller中的注册状态. 
};