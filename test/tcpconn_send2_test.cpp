#include <cstdio>

#include "logger.h"
#include "tcp_server.h"
#include "buffer.h"
#include "timestamp.h"

using namespace std;
/* 测试TcpConnection的send() */
string msg1(100, 'A');
string msg2(200, 'B'); 

void onConnection(const TcpConnection::TcpConnectionPtr& conn) {
    if (conn->connected()) {
        printf("onConnection(): new connection [%s] from %u\n", 
            conn->getName().c_str(), 
            conn->getPeerAddress().getPort());
        conn->send(msg1); 
        conn->send(msg2);
        conn->shutdown();  
    } else {
        printf("onConnection(): connection [%s] is down\n",
            conn->getName().c_str());
    }
}

void onMessage(const TcpConnection::TcpConnectionPtr& conn, Buffer* buf, Timestamp receive_time) {
    printf("onMessage() : received %zd bytes from connection [%s], at: %s\n",
         buf->readableBytes(), 
         conn->getName().c_str(), 
         receive_time.toFormattedString().c_str());
    std::string msg = buf->retrieveAllToString();
    printf("onMessage():[%s]\n", msg.c_str());
}


int main() {
    InetAddress listen_addr(8899); 
    EventLoop loop;
    TcpServer server(&loop, listen_addr); 
    Logger::setLogLevel(Logger::TRACE); 
    server.setConnectionCallback(onConnection); 
    server.setMessageCallback(onMessage); 
    server.start();

    loop.loop();
}
