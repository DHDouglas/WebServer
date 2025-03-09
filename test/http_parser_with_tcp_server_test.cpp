#include <cstdio>

#include "logger.h"
#include "tcp_server.h"
#include "buffer.h"
#include "timestamp.h"
#include "http_parser.h"

using namespace std;

/* 测试在TcpServer之上, 使用HttpParser从TcpConnection接收缓冲区中解析报文 */

HttpParser http_parser; 
 
void onConnection(const TcpConnection::TcpConnectionPtr& conn) {
    if (conn->connected()) {
        printf("onConnection(): tid=%d new connection [%s] from %u\n", 
            CurrentThread::getTid(), 
            conn->getName().c_str(), 
            conn->getPeerAddress().getPort());
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

    size_t size = buf->readableBytes();
    size_t parsed_size = 0;
    auto ret = http_parser.parse(buf->peek(), size, parsed_size); 
    // 解析得到Http报文: 
    if (ret == HttpParser::ParseResult::SUCCESS) {
        assert(parsed_size == size);  
        printf("onMessage():[%s]\n", http_parser.encode().c_str());
    }
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
