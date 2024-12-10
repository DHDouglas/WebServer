#pragma once
#include <sys/socket.h>
#include <netinet/in.h>

#include "../buffer/SimpleExam/buffer.h"
#include "http_parser.h"


// 实际上, 这个对象应该叫TCPConn, 或者CommonConn, 而不是HttpConn. 
class HttpConn {
public:
    HttpConn();
    ~HttpConn(); 

    void init(int sockfd, const sockaddr_in& addr); 

    // 从sockfd中读取数据, 存入缓冲区. 
    ssize_t read(int* saveErrono);
    // 将缓冲区中的数据向socketfd中写入. 
    ssize_t write(int *saveErrno); 

    // 解析缓冲区中的内容, 进行处理. 
    bool process(); 

    // 处理请求
    bool deal_request(); 

    int m_sockfd;                // 该连接的套接字描述符    
private:
    struct sockaddr_in m_addr;  
    bool m_is_close;   

    // 读写缓冲区
    Buffer m_inputBuffer;
    Buffer m_outputBuffer;
    HttpParser parser;   
};
