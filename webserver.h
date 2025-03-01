#pragma once

#include <sys/epoll.h>
#include "utils.h"
#include "threadpool/threadpool.h"
#include "http/http_conn.h"
#include <memory>

const int MAX_EVENT_NUMBER = 10000;   // 最大epoll事件数.  ? 这里



class WebServer {
public:
    WebServer(int port, bool opt_linger, int num_threads); 
    ~WebServer(); 

    void init();  
    void eventListen(); 
    void eventLoop(); 
    bool deal_conn();
    void deal_read(int fd); 
    void deal_write(int fd); 
    void close_conn(int fd); 
    void OnRead_(HttpConn* conn); 
    void OnWrite(HttpConn* conn); 


private:

    Utils utils;   // 辅助类

    int m_port;        // 监听端口
    bool m_OPT_LINGER;  // 是否开启 SO_LINGER套接字选项

    std::unique_ptr<ThreadPool> m_threadpool;  
    std::unordered_map<int, HttpConn> m_users;  // HttpConn实例. 

    int m_listenfd;    // 监听描述符.
    int m_epollfd;     // epoll实例的描述符.
    struct epoll_event events[MAX_EVENT_NUMBER]; 
};

