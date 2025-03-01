#pragma once

#include <unistd.h>
#include <sys/epoll.h>
#include <functional>

class EventLoop;

class Channel {
public :
    using CallBack = std::function<void()>; 

    Channel(EventLoop* loop, int fd);
    ~Channel(); 
    
    void setEvents(int evt); 
    void setRevents(int evt); 
    void handleEvents();
    int getFd() const; 
    int getEvents() const; 

    void addToEpoller();
    void modInEpoller();
    void delFromEpoller(); 

    void setReadCallBack(CallBack cb);
    void setWriteCallBack(CallBack cb); 
    void setErrorCallBack(CallBack cb); 


private:
    CallBack readCallBack_; 
    CallBack writeCallBack_; 
    CallBack errorCallBack_; 

    EventLoop* loop_;   // 每个channel只属于1个eventloop(IO线程)
    bool addInEpoller;  
    const int fd_;     // 始终只关联于一个fd.
    int events_;       // bit pattern: 监听事件集
    int revents_;      // bit pattern: 当前活跃事件集
    int idx_;          // used by Poller
};