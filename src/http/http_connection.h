#pragma once 

#include <memory>
#include <string>

#include "buffer.h"
#include "timer.h"
#include "http_message.h"
#include "http_parser.h"
#include "timing_wheel.h"

class TcpConnection;

class HttpConnection {
public:
    using ParseResult = HttpParser::ParseResult; 
    using TimerWeakPtr = TimingWheel::EntryWeakPtr;
    using TcpConnectionWeakPtr = std::weak_ptr<TcpConnection>;

public:
    HttpConnection(TcpConnectionWeakPtr tcp_conn);
    ~HttpConnection();
    // 处理&解析HTTP请求报文
    void handleMessage(Buffer* buf);

    TcpConnectionWeakPtr getTcpConnectionWeakPtr() const { return tcp_conn_wkptr; }
    TimerWeakPtr getTimerWeakPtr() const { return timer_wkptr_; }

    static void setRootPath(std::string root_path);
    static void setTimeout(int seconds); 
    // 定时器回调.
    static void onTimer(Any& context);

private:
    // 根据URL执行不同业务逻辑 
    void handleRequest();
    // 请求静态资源
    void responseMsg(const char* msg, size_t len, HttpStatusCode code);
    void responseFile(const string& path, HttpStatusCode code);  
    // 错误响应        
    void errorResponse(HttpStatusCode code);
    // 发送报文
    void sendResponse(const HttpResponse& response);     // base.
    // 仅关闭写端. 
    void shutdown() const;
    // 断开连接
    void forceClose() const;

    // 重置定时器
    void restartTimer(); 
    // 移除定时器
    void removeTimer();

private:
    HttpParser parser_;
    TcpConnectionWeakPtr tcp_conn_wkptr; 
    TimerWeakPtr timer_wkptr_; 
    static std::string root_path_; 
    static int timeout_seconds_;  
};


// 映射文件到内存. RAII封装
struct MmapData {
    explicit MmapData(): fd_(-1), addr_(nullptr), size_(0) { }
    ~MmapData();
    bool mmap(const std::string& file_path); 
    int fd_;
    void* addr_;
    size_t size_;
};
