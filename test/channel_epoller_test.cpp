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
    channel.setReadCallback(timeout);  // 注册read回调
    channel.enableReading();           // 注册到epoll监听读事件

    struct itimerspec howlong;
    bzero(&howlong, sizeof howlong);
    howlong.it_value.tv_sec = 5;
    timerfd_settime(timerfd, 0, &howlong, NULL); 
    loop.loop(); 
    // 从loop的epoller中移除channel. 否则channel析构时abort.
    channel.disableAll(); 
    channel.removeFromEpoller();

    close(timerfd);
}
