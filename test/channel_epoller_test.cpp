#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <cstdio>
#include <cstring>
#include <unistd.h>

#include "eventloop.h"
#include "channel.h"
#include "timestamp.h"

EventLoop* g_loop;

// 测试Epoller的epoll功能以及channel的事件分发功能. 

void timeout(Timestamp receive_time) {
    printf("%s Timeout\n", receive_time.toFormattedString().c_str()); 
    g_loop->quit(); 
}

int main() {
    printf("%s started\n", Timestamp::now().toFormattedString().c_str()); 
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
