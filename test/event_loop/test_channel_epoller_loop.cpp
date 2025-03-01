#include <bits/types/struct_itimerspec.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <cstdio>
#include <cstring>

#include "eventloop.h"
#include "channel.h"

EventLoop* g_loop;

// 测试

void timeout() {
    printf("Timeout\n");
    g_loop->quit(); 
}

int main() {
    EventLoop loop;
    g_loop = &loop; 

    int timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    // 创建channel, 绑定fd
    Channel channel(&loop, timerfd); 
    channel.setReadCallBack(timeout);  // 注册read回调
    int events = EPOLLIN | EPOLLPRI; 
    channel.setEvents(events);    // 设置监听事件
    channel.addToEpoller();       // 在其所属eventloop绑定的Epoller中注册. 

    struct itimerspec howlong;
    bzero(&howlong, sizeof howlong);
    howlong.it_value.tv_sec = 5;
    timerfd_settime(timerfd, 0, &howlong, NULL); 
    loop.loop(); 

    close(timerfd);
}
