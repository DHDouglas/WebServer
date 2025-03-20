#pragma once

#include <vector>
#include <map>

#include "channel.h"
#include "timestamp.h"

class EventLoop;

class Epoller {
public:
    using EventList = std::vector<struct epoll_event>; 
    using ChannelMap = std::map<int, Channel*>; 
    using ChannelList = std::vector<Channel*>; 

    Epoller(EventLoop* loop);
    ~Epoller(); 
    Timestamp poll(int timeoutMs, ChannelList* activeChannels); 

    bool hasChannel(Channel* channel); 
    int getEpollFd() const; 
    void updateChannel(Channel* channel);  // 更新fd及其channel
    void removeChannel(Channel* channel); 
    void assertInLoopThread() const; 


private:
    enum Operator { ADD, MOD, DELETE }; 
    void epollUpdate(int epoll_op, Channel* channel); 
    void fillActiveChannels(int nfds, ChannelList* activeChannels) const;        // 将events_中活动fd填入activeChannels.


private:
    static const int kInitEventListSize = 16; 

    EventLoop* owner_loop_;
    int epoll_fd_;            // epoll 实例
    EventList events_;        // 供epoll_wait使用的epoll_event数组
    ChannelMap channels_;   
};