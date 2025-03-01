#pragma once

#include <sys/epoll.h>
#include <vector>
#include <map>
#include <cstdio>
#include <cstdlib>
#include <cassert>

#include "channel.h"

class EventLoop;


class Epoller {
public:
    using EventList = std::vector<struct epoll_event>; 
    using ChannelMap = std::map<int, Channel*>; 
    using ChannelList = std::vector<Channel*>; 

    Epoller(EventLoop* loop);
    ~Epoller(); 
    void poll(int timeoutMs, ChannelList* activeChannels); 
    void fillActiveChannels(int nfds, ChannelList* activeChannels) const;        // 将events_中活动fd填入activeChannels.

    int getEpollFd() const; 
    void epollMod(Channel* channel);   // 注册fd及其channel
    void epollAdd(Channel* channel);   // 更新fd及其channel
    void epollDel(Channel* channel); 
    void assertInLoopThread() const; 

private:
    EventLoop* owner_loop_;
    int epoll_fd_;            // epoll 实例
    EventList events_;        // 供epoll_wait使用的epoll_event数组
    ChannelMap channels_;   
};