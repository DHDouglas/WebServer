#include <cstdio>

#include "logger.h"
#include "tcp_server.h"

void onConnection(const TcpConnection::TcpConnectionPtr& conn) {
    if (conn->connected()) {
        printf("onConnection(): new connection [%s] from %u\n", 
            conn->getName().c_str(), 
            conn->getPeerAddress().getPort());
    } else {
        printf("onConnection(): connection [%s] is down\n",
            conn->getName().c_str());
    }
}

void onMessage(const TcpConnection::TcpConnectionPtr& conn, const char* data, ssize_t len) {
    printf("onMessage() : received %zd bytes from connection [%s], data: %s\n",
         len, conn->getName().c_str(), data);
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
