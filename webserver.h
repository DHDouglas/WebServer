#pragma once

#include <sys/epoll.h>
#include "utils.h"

const int MAX_EVENT_NUMBER = 10000;   // 最大epoll事件数.  ? 这里



class WebServer {
public:
    WebServer(int port, bool opt_linger); 

    void init();  
    void eventListen(); 
    void eventLoop(); 
    bool deal_conn();
    void deal_read(); 


private:

    Utils utils;   // 辅助类

    int m_port;        // 监听端口
    bool m_OPT_LINGER;  // 是否开启 SO_LINGER套接字选项


    int m_listenfd;    // 监听描述符.
    int m_epollfd;     // epoll实例的描述符.
    struct epoll_event events[MAX_EVENT_NUMBER]; 
};

