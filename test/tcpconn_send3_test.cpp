#include <cstdio>

#include "logger.h"
#include "tcp_server.h"
#include "buffer.h"
#include "timestamp.h"

using namespace std;
/* 测试向已关闭的conn_fd写入,  触发SIGPIPE. */
string msg1(100, 'A');
string msg2(200, 'B'); 
int sleep_seconds = 5;
 
void onConnection(const TcpConnection::TcpConnectionPtr& conn) {
    if (conn->connected()) {
        printf("onConnection(): tid=%d new connection [%s] from %u\n", 
            CurrentThread::getTid(), 
            conn->getName().c_str(), 
            conn->getPeerAddress().getPort());
        if (sleep_seconds > 0) {
            ::sleep(sleep_seconds); 
        }
        conn->send(msg1); 
        conn->send(msg2);
        conn->shutdown();  
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
    std::string msg = buf->retrieveAllToString();
    printf("onMessage():[%s]\n", msg.c_str());
}


int main() {
    printf("main(): pid = %d\n", getpid());

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
