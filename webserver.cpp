#include "webserver.h"
#include "http/http_conn.h"
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
#include <arpa/inet.h>




WebServer::WebServer(int port, bool opt_linger, int num_threads)
: m_port(port), m_OPT_LINGER(opt_linger), m_threadpool(std::make_unique<ThreadPool>(num_threads)) {
    printf("Initialize Server\n"); 
    
}

WebServer::~WebServer() {
    // close(m_listenfd);
    // close(m_epollfd); 

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

        printf("process\n");
        for (int i = 0; i < nfds; ++i) {
            /* 根据事件来源, 区分几种情况:
             * 来自监听描述符listn_fd: 新连接, 需调用accept
             * 来自已连接的描述符conn_fd: 需要进行I/O处理
             * 
             */
            int fd = events[i].data.fd;  // 发生事件的描述符
            if (fd == m_listenfd) {      // 监听描述符有新事件
                // 处理新连接(此处连接建立过程仍然由主线程完成, 没有分配给线程池中工作线程). 
                if (false == deal_conn()) {
                    continue;
                }
            } else if (events[i].events & (EPOLLHUP | EPOLLRDHUP | EPOLLERR)) {
                // 断开连接
                close_conn(fd); 
                
            } else if (events[i].events & EPOLLIN) {
                // 处理客户端发来的数据; 
                deal_read(fd);  
            }
        }
        sleep(2); 
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

    // 将conn_fd将入监听, 并设置为非阻塞. 
    epoll_event ev {0};
    ev.events = EPOLLIN; 
    ev.data.fd = conn_fd; 
    epoll_ctl(m_epollfd, EPOLL_CTL_ADD, conn_fd, &ev); 
    utils.setnonblockling(conn_fd); 

    // 为该连接关联一个HttpConn实例. 
    m_users[conn_fd].init(); 


    // 打印客户端的IP地址和端口号. 
    int buf_size = 1024; 
    char buffer[buf_size]; 
    printf("Connection from:  %s:%d\n", 
        inet_ntop(AF_INET, &addr.sin_addr, buffer, sizeof(buffer)), 
        ntohs(addr.sin_port));
    return true;
}


void WebServer::close_conn(int fd) {
    printf("Close fd: %d\n", fd); 
    epoll_ctl(m_epollfd, EPOLL_CTL_DEL, fd, NULL);  // 从epoll实例中移除对该描述符的监听. 
    close(fd); 
}


void WebServer::deal_read(int fd) {
    // 检测到读事件, 将事件放入线程池的请求队列. 
    m_threadpool->submit([fd]() -> void{ 
        printf("gggg\n"); 
        const char* str = "Hello world"; 
        send(fd, str, strlen(str), 0);  
    });

    printf("Received: \n");
    // int buf_size = 1024; 
    // char buffer[buf_size]; 
    // ssize_t n = read(fd, buffer, buf_size); 
    // buffer[n] = '\0'; 
    // printf("%s\n", buffer);
    // const char* str = "Hello world"; 
    // send(fd, str, strlen(str), 0);  
}

void WebServer::deal_write(int fd) {
    // 向线程池中插入写任务. 
    // 应该向线程池里存入哪些任务? 针对Http协议, 应当封装一个Http对象, 涵盖所有请求? 
    // threadpool->AddTask(); 

}
