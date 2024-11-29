#include "webserver.h"
#include <asm-generic/socket.h>
#include <cerrno>
#include <cstdlib>
#include <sys/epoll.h>
#include <unistd.h>
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cassert>
#include <cstring>




WebServer::WebServer(int port, bool opt_linger): m_port(port), m_OPT_LINGER(opt_linger) {
    printf("Initialize Server\n"); 
}


void WebServer::init() {
    // TODO:
}




void WebServer::eventListen() {
    // 服务器端基本步骤: socket, bind, listen. 

    int ret; 
    // 初始化: 创建监听socket. 
    m_listenfd = socket(PF_INET, SOCK_STREAM, 0);  
    assert(m_listenfd >= 0); 

    // 优雅关闭: 调用close后等待所剩数据发送完毕.
    struct linger opt_linger = {0}; 
    if (m_OPT_LINGER) {
        opt_linger.l_onoff = 1;
        opt_linger.l_linger = 1;
        ret = setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &opt_linger, sizeof(opt_linger)); 
        assert(ret == 0); 
    }
    // 允许地址复用
    int optval = 1; 
    ret = setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));  
    assert(ret >= 0); 

    // 绑定地址和端口
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr); 
    bzero(&addr, addr_len); 
    addr.sin_family = AF_INET;  // IPv4
    addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    addr.sin_port = htons(m_port); 
    ret = bind(m_listenfd, (struct sockaddr*)&addr, addr_len); 
    assert(ret >= 0); 
    // 开启监听
    ret = listen(m_listenfd, 5);  


    // 创建epoll实例. 
    m_epollfd = epoll_create1(0); 
    // 向该epoll实例中, 注册监听的描述符集以及监听事件. 
    struct epoll_event ev; 
    ev.events = EPOLLIN;
    ev.data.fd = m_listenfd; 
    if (epoll_ctl(m_epollfd, EPOLL_CTL_ADD, m_listenfd, &ev) == -1) {
        perror("epoll_ctl"); 
        close(m_listenfd); 
        close(m_epollfd);
        exit(EXIT_FAILURE); 
    }
    // 设置监听描述符非阻塞
    utils.setnonblockling(m_listenfd);  
}


void WebServer::eventLoop() {
    // 主循环: 基于epoll的I/O多路复用.

    // 用一个变量, 控制主循环的结束.
    bool stop_server = false; 

    while (!stop_server) {
        int nfds = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);  
        // 错误检查
        if (nfds == -1 && errno != EINTR) {
            printf("epoll faliture\n"); 
            break;
        }

        for (int i = 0; i < nfds; ++i) {
            /* 根据事件来源, 区分几种情况:
             * 来自监听描述符listn_fd: 新连接, 需调用accept
             * 来自已连接的描述符conn_fd: 需要进行I/O处理
             * 
             */
            int fd = events[i].data.fd;  // 发生事件的描述符
            if (fd == m_listenfd) {      // 监听描述符有新事件
                // 处理新连接
                if (false == deal_conn()) {
                    continue;
                }
            } else if (events[i].events & EPOLLIN) {
                // 处理客户端发来的数据; 
                deal_read();  
            }   
        }

        // 处理定时. 
    }
}


bool WebServer::deal_conn() {
    // 如果是边缘出发, 则需要在一个循环中, 持续处理监听连接, 因为可能会有多个客户端同时到达.  
    // 1) 调用accept建立连接, 返回conn_fd; 
    // 2) 将conn_fd设置为非阻塞;
    // 3) 将conn_fd添加到epoll实例中, 进行监听. 

    struct sockaddr_in addr; 
    socklen_t addr_len = sizeof(sockaddr_in); 

    int conn_fd = accept(m_listenfd, (struct sockaddr*)&addr, &addr_len); 
    if (conn_fd == -1) {
        perror("accept");
        return false; 
    }

    utils.setnonblockling(conn_fd); 

    epoll_event ev {0};
    ev.events = EPOLLIN; 
    ev.data.fd = conn_fd; 
    epoll_ctl(m_epollfd, EPOLL_CTL_ADD, conn_fd, &ev); 

    return true;
}


void WebServer::deal_read() {
    printf("gggg\n");
}
