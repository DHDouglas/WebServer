#include <cstdio>
#include <unistd.h>

#include "any.h"
#include "http_connection.h"
#include "eventloop.h"
#include "logger.h"
#include "tcp_server.h"
#include "buffer.h"
#include "thread.h"
#include "timestamp.h"

void onConnection(const TcpConnection::TcpConnectionPtr& conn) {
    if (conn->connected()) {
        printf("onConnection(): tid=%d new connection [%s] from %u\n", 
            CurrentThread::getTid(), 
            conn->getName().c_str(), 
            conn->getPeerAddress().getPort());
        conn->setContext(make_shared<HttpConnection>(conn));
    } else {
        printf("onConnection(): tid=%d connection [%s] is down\n",
            CurrentThread::getTid(), 
            conn->getName().c_str());
    }
}

void onMessage(const TcpConnection::TcpConnectionPtr& conn, Buffer* buf, Timestamp receive_time) {
    printf("onMessage() : tid=%d received %zd bytes from connection [%s], at: %s\n",
        CurrentThread::getTid(),
        buf->readableBytes(), 
        conn->getName().c_str(), 
        receive_time.toFormattedString().c_str());
    printf("onMessage():[%s]\n", buf->retrieveAllToString().c_str() ); 
    auto http_conn = any_cast<std::shared_ptr<HttpConnection>>(conn->getMutableContext());
    (*http_conn)->handleMessage(buf);
}


int main() {
    InetAddress listen_addr(8899); 
    EventLoop loop;
    TcpServer server(&loop, listen_addr); 
    Logger::setLogLevel(Logger::TRACE); 
    server.setConnectionCallback(onConnection); 
    server.setMessageCallback(onMessage); 
    int num_thread = 5; 
    server.setThreadNum(num_thread); 
    server.start();
    loop.loop();
}