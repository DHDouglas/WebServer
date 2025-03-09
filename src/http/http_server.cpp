#include "http_server.h"
#include "http_connection.h"
#include "http_message.h"
#include "http_parser.h"
#include "inet_address.h"
#include "logger.h"
#include "tcp_server.h"


#include <cerrno>
#include <cstddef>
#include <fcntl.h>
#include <linux/limits.h>    // for PATH_MAX
#include <sys/mman.h>
#include <sys/stat.h>
#include "timestamp.h"


using namespace std;

HttpServer::HttpServer(const string ip, 
                       const int port, 
                       int num_thread,
                       string root_path,
                       double timeout_seconds)
    : addr_(ip.empty() ? InetAddress(port) : InetAddress(ip, port)),
    root_path_(root_path),
    timeout_duration_(Timestamp::secondsToDuration(timeout_seconds)),
    tcp_server_(&loop_, addr_, "HttpServer")
{   
    processResourcesRootPath(); 
    tcp_server_.setThreadNum(num_thread); 
    tcp_server_.setConnectionCallback(
        bind(&HttpServer::onConnection, this, placeholders::_1)); 
    tcp_server_.setMessageCallback(
        bind(&HttpServer::onMessage, this, placeholders::_1,
             placeholders::_2, placeholders::_3));
}


void HttpServer::processResourcesRootPath() {
    // TODO: 区分是相对路径还是绝对路径. 
    // 如果是绝对路径, 直接赋给root_path_;
    // 如果是相对路径, 则调用getcwd获取当前工作目录, 再concat.
}

void HttpServer::start() {
    LOG_INFO << "HttpServer starts listening on " << tcp_server_.getIpPort();
    tcp_server_.start();
    loop_.loop();
}


void HttpServer::onConnection(const TcpServer::TcpConnectionPtr& tcp_conn) {
    if (tcp_conn->connected()) {
        LOG_TRACE << "HttpServer::onConnection: new HttpConnection"; 
        auto http_conn = make_shared<HttpConnection>(tcp_conn, timeout_duration_); 
        maps_[tcp_conn] = http_conn;
    } else {
        maps_.erase(tcp_conn); 
        LOG_TRACE << "HttpServer::onConnection: remove tcp_conn, remainder: " << maps_.size(); 
    }
}


void HttpServer::onMessage(const TcpServer::TcpConnectionPtr& tcp_conn,
                           Buffer* buf, 
                           Timestamp receive_time)
{
    LOG_TRACE << "HttpServer::onMessage receive_time: " << receive_time.toFormattedString().c_str(); 

    auto http_conn = maps_[tcp_conn]; 
    size_t size = buf->readableBytes(); 
    size_t parsed_size = 0; 
    HttpParser& parser = http_conn->parser(); 
    auto ret = parser.parse(buf->peek(), size, parsed_size);
    buf->retrieve(parsed_size); 
    using R = HttpConnection::ParseResult; 

    if (ret == R::SUCCESS) {
        LOG_INFO << "HttpServer::onMessage http request parsing successed"; 
        onRequest(tcp_conn, http_conn);
    } else if (ret == R::ERROR) {
        LOG_WARN << "HttpServer::onMessage http request parsing error"; 
        tcp_conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
        tcp_conn->shutdown();
    } else {
        LOG_INFO << "HttpServer::onMessage http request parsing continue"; 
    }
}

void HttpServer::onRequest(const TcpServer::TcpConnectionPtr& tcp_conn,  const HttpConnectionPtr& http_conn) {
    LOG_INFO << http_conn->getRequestAsString();
    const HttpParser& parser = http_conn->parser(); 
    HttpResponse response; 
    HttpStatusCode code = HttpStatusCode::OK; 
    MmapData mmap_data; 


    // TODO: 解析url.
    // string url = "test.txt"; 
    string path = parser.getUri(); 

    code = processPath(path);  
    // if (code != HttpStatusCode::OK) {
        // 返回错误报文
    // }

    // TODO: 根据文件后缀填充字段类型(Content-Type)
    
    // 区分HEAD与GET, GET不处理
    if (code == HttpStatusCode::OK && !strncasecmp(parser.getMethod(), "GET", 3)) {
        // 请求的资源文件由mmap映射到进程内存中.
        bool ret = mmap_data.mmap(path); 
        if (ret) {
            response.setBody(mmap_data.addr_, mmap_data.size_);  // 自动添加"Content-Length"头部
        } else {
            code = HttpStatusCode::InternalServerError;  // 500
        }
    }
    
    // 构造响应报文
    response.setVersion(parser.getVersion());
    response.setStatus(code);
    response.addHeader("Content-Length", to_string(response.getBodySize()));   // 自动填充Content-Length头
    if (!parser.getKeepAlive()) {
        response.addHeader("Connection", "close"); 
    }
    // 发送响应报文
    sendResponse(tcp_conn, response); 

    // 短连接下, 服务器端回发响应报文后关闭写端.
    if (!parser.getKeepAlive()) {
        tcp_conn->shutdown();
    }
    // 服务器内部错误, 考虑关闭整个连接?
    string code_str = getHttpStatusCodeString(code); 
    if (code_str[0] == '5') {
        tcp_conn->forceClose();
    }
}


// void HttpServer::parseUrl(const string& url) {
//     // 1) 提取url √parser已提取
//     // 2) 检查url路径, 得到real_path, 若为目录则补上完整地址 √
//     // 3) 根据real_path, 检查路径是否有效, 目标文件是否存在等等. 若无效, 设置错误状态码
//     // 4) 若有效, 以赋给形参real_path. 否则置空. 
// }


HttpStatusCode HttpServer::processPath(string& path) {
    assert(!path.empty() && path[0] == '/'); 
    string real_path; 
    if (root_path_.back() == '/') {
        real_path = root_path_ + path.substr(1);
    } else {
        real_path = root_path_ + path; 
    }
    // 路径解析, 得到绝对路径
    char resolved_path[PATH_MAX]; 
    if (realpath(real_path.c_str(), resolved_path) == nullptr) {
        // code = 404; 
        return HttpStatusCode::NotFound;  
    }
    real_path = resolved_path;

    // 检查目标路径是否仍在资源目标下. 
    if (real_path.compare(0, root_path_.length(), root_path_) != 0) {
        // code = 403; 禁止访问资源目录外的路径
        return HttpStatusCode::Forbidden; 
    }

    // TODO: 若为目录, 且为该目录指定有默认文件, 则加上默认文件名.

    // 检查文件属性
    struct stat file_stat {}; 
    if (stat(real_path.c_str(), &file_stat) == -1 || S_ISDIR(file_stat.st_mode)) { 
        LOG_SYSERR << "fstat failed, file: " << real_path;
        // code = 404;  文件不存在
        return HttpStatusCode::NotFound; 
    } else if (!(file_stat.st_mode & S_IROTH)) {
        // code = 403; 
        return HttpStatusCode::Forbidden;
    }
    path = real_path; 
    return HttpStatusCode::OK; 
}


void HttpServer::sendResponse(const TcpServer::TcpConnectionPtr& tcp_conn, const HttpResponse& response) {
    // 临时用栈空间作为Http的应用层缓冲区, 只存放请求行+头部.
    Buffer buf;  
    response.encode(&buf); 
    tcp_conn->send(&buf);
}


bool HttpServer::MmapData::mmap(const string& file_path) {
    fd_ = open(file_path.c_str(), O_RDONLY);
    if (fd_ == -1) {
        LOG_SYSERR << "MmapData::MmapData open failed, file : " << file_path; 
        return false;
    }
    struct stat file_stat {};  // 值初始化, 清0 
    if (fstat(fd_, &file_stat) == -1) {
        LOG_SYSERR << "MmapData::MmapData fstat failed, file: " << file_path << " fd:" << fd_;
        close(fd_); 
        fd_ = -1;
        return false;
    }
    size_ = file_stat.st_size; 
    addr_ = ::mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, fd_, 0); // 只读
    if (addr_ == MAP_FAILED) {
        LOG_SYSERR << "MmapData::MmapData mmap file failed. file: " << file_path << " fd:" << fd_
                   << " errno: " <<strerror_tl(errno);
        addr_ = nullptr;
        close(fd_); 
        fd_ = -1;
        return false; 
    }
    return true;
}

HttpServer::MmapData::~MmapData() {
    if (addr_) {
        munmap(addr_, size_);
    }
    if (fd_ != -1) {
        close(fd_);
    }
}