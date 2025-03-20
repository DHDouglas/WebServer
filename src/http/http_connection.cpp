#include "http_connection.h"

#include <climits>
#include <string>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/limits.h>    // for PATH_MAX
#include <sys/mman.h>
#include <sys/stat.h>
#include <cassert>
#include <unordered_map>

#include "tcp_connection.h"
#include "eventloop.h"
#include "http_message.h"
#include "logger.h"


const unordered_map<HttpStatusCode, string> ERROR_RET_FILE = {
    { HttpStatusCode::BadRequest, "/400.html" },
    { HttpStatusCode::Forbidden, "/403.html" },
    { HttpStatusCode::NotFound, "/404.html"},
    { HttpStatusCode::InternalServerError, "/500.html"},
};

const unordered_map<string, string> DEFAULT_FILE{
    {"/",         "/index.html"},
    {"/register", "/register.html"},
    {"/login",    "/login.html"},
    {"/welcome",  "/welcome.html"},
    {"/video",    "/video.html"}, 
    {"/picture",  "/picture.html"}
};

string getErrorCodeRerturnFile(HttpStatusCode code) {
    auto it =  ERROR_RET_FILE.find(code);
    if (it == ERROR_RET_FILE.end()) {
        return ""; 
    }
    return it->second; 
}

static vector<char> FIX_MSG(5 * 1024, 'A');  // 5KB

std::string HttpConnection::root_path_; 
int HttpConnection::timeout_seconds_;  

void HttpConnection::setRootPath(std::string root_path) {
    root_path_ = std::move(root_path);
}

void HttpConnection::setTimeout(int seconds) {
    timeout_seconds_ = seconds; 
}

HttpConnection::HttpConnection(TcpConnectionWeakPtr tcp_conn) 
    : tcp_conn_wkptr(tcp_conn)
{
    // 新增定时器
    if (timeout_seconds_ > 0) {
        if (auto conn_sptr = tcp_conn.lock()) {
            auto wheel = any_cast<unique_ptr<TimingWheel>>(
                conn_sptr->getOwnerLoop()->getMutableContext()
            );
            timer_wkptr_ = (*wheel)->insert(tcp_conn); 
        }
    }
}

void HttpConnection::onTimer(Any& context) {
    auto conn_wkptr = any_cast<weak_ptr<TcpConnection>>(&context); 
    if (auto conn_sptr = (*conn_wkptr).lock()) {
        conn_sptr->forceClose(); 
    }
}

HttpConnection::~HttpConnection() {
    LOG_TRACE << "HttpConnection::~HttpConnection this::" <<  this 
              << " tcp_conn_wkptr use count: " << tcp_conn_wkptr.use_count();
    removeTimer();
}

void HttpConnection::shutdown() const {
    if (auto conn_sptr = tcp_conn_wkptr.lock()) {
        conn_sptr->shutdown(); 
    }
}

void HttpConnection::forceClose() const {
    if (auto conn_sptr = tcp_conn_wkptr.lock()) {
        conn_sptr->forceClose(); 
    }
}

void HttpConnection::restartTimer() {
    if (auto timer = timer_wkptr_.lock()) {
        timer->owner_wheel_->update(timer); 
    }
}

void HttpConnection::removeTimer() {
    if (auto timer = timer_wkptr_.lock()) {
        timer->owner_wheel_->remove(timer); 
    }
}

void HttpConnection::handleMessage(Buffer* buf) {
    restartTimer(); 
    size_t size = buf->readableBytes(); 
    size_t parsed_size = 0; 
    auto ret = parser_.parse(buf->peek(), size, parsed_size);
    buf->retrieve(parsed_size); 

    using R = HttpConnection::ParseResult; 
    if (ret == R::SUCCESS) {
        // 解析得到完整HTTP请求报文, 处理请求.
        LOG_TRACE << "HttpServer::onMessage http request parsing successed"; 
        handleRequest();
    } else if (ret == R::ERROR) {
        LOG_WARN << "HttpServer::onMessage http request parsing error"; 
        errorResponse(HttpStatusCode::BadRequest);
        forceClose();
    } else {
        LOG_TRACE << "HttpServer::onMessage http request parsing continue"; 
    }
}

void HttpConnection::handleRequest() {
    LOG_TRACE << parser_.encode(); 
    string path = parser_.getUri(); 
    // For benchmark testing.
    if (path == "/benchmark") {
        responseMsg("Hello World!", 12, HttpStatusCode::OK);  // 不取文件, 省略磁盘I/O
        return; 
    }
    if (path == "/benchmark-5KB") {
        responseMsg(FIX_MSG.data(), FIX_MSG.size(), HttpStatusCode::OK);  // 5KB
        return;
    }
    if (path == "/nocontent") {
        responseMsg(nullptr, 0, HttpStatusCode::NoContent);
        return;
    }

    // URL匹配默认文件
    auto it = DEFAULT_FILE.find(path);
    if (it != DEFAULT_FILE.end()) {
        path = it->second;
    }
    // 请求静态资源
    responseFile(path, HttpStatusCode::OK); 
}


void HttpConnection::responseMsg(const char* msg, size_t len, HttpStatusCode code) {
    HttpResponse response; 
    HttpMethod method = getHttpMethodAsEnum(parser_.getMethod()); 
    if (method != HttpMethod::HEAD && method != HttpMethod::GET) {
        return errorResponse(HttpStatusCode::NotImplemented); 
    }

    response.setVersion(parser_.getVersion());
    response.setStatus(code);
    if (msg && len > 0) {
        assert(strlen(msg) == len); 
        if (method == HttpMethod::GET) {
            response.setBody(msg, len);  
        }
        response.addHeader("Content-Type", "text/plain");
        response.addHeader("Content-Length", to_string(len)); 
    }
    if (!parser_.isKeepAlive()) {
        response.addHeader("Connection", "close"); 
    }
    LOG_TRACE << response.encode();
    sendResponse(response); 
}


void HttpConnection::responseFile(const string& path, HttpStatusCode code) {
    // 构造响应报文
    HttpResponse response; 
    MmapData mmap_data; 

    HttpMethod method = getHttpMethodAsEnum(parser_.getMethod()); 
    if (method != HttpMethod::HEAD && method != HttpMethod::GET) {
        return errorResponse(HttpStatusCode::NotImplemented); 
    }

    if (!path.empty() && path[0] == '/') {
        string real_path = root_path_ + path; 
        // 路径解析, 得到绝对路径
        char resolved_path[PATH_MAX]; 
        if (realpath(real_path.c_str(), resolved_path) == nullptr) {
            return errorResponse(HttpStatusCode::NotFound); 
        }
        real_path = resolved_path;
        // 检查目标路径是否仍在资源目标下. 
        if (real_path.compare(0, root_path_.length(), root_path_) != 0 ||
            (real_path.size() > root_path_.size() && real_path[root_path_.size()] != '/')) {
            return errorResponse(HttpStatusCode::Forbidden); 
        }
        // 检查文件属性
        struct stat file_stat {}; 
        if (stat(real_path.c_str(), &file_stat) == -1 || S_ISDIR(file_stat.st_mode)) { 
            LOG_TRACE << "HttpConnection::requestFile fstat failed, file: " << real_path;
            return errorResponse(HttpStatusCode::NotFound); 
        } else if (faccessat(AT_FDCWD, real_path.c_str(), R_OK, AT_EACCESS) != 0) {
            LOG_TRACE << "HttpConnection::requestFile faccessat failed, file: " << real_path;
            return errorResponse(HttpStatusCode::Forbidden); 
        }
        // 获取文件大小与文件内容
        LOG_TRACE << real_path;
        if (method == HttpMethod::HEAD) {
            response.setBody(nullptr, file_stat.st_size);
        } else {
            if (mmap_data.mmap(real_path)) {
                response.setBody(mmap_data.addr_, mmap_data.size_);
            } else {
                return errorResponse(HttpStatusCode::InternalServerError);
            }
        }
        response.addHeader("Content-Type", getContentType(real_path));  // 根据文件后缀确认Content-Type.
    }

    response.setVersion(parser_.getVersion());
    response.setStatus(code);
    response.addHeader("Content-Length", to_string(response.getBodySize())); 
    if (!parser_.isKeepAlive()) {
        response.addHeader("Connection", "close"); 
    }
    LOG_TRACE << response.encode();
    sendResponse(response); 
}


void HttpConnection::errorResponse(HttpStatusCode code) {
    LOG_TRACE << getHttpStatusCodeString(code); 
    HttpResponse response; 
    MmapData mmap_data;
    // 根据错误状态码, 获取对应返回的HTML文件: 
    string error_html_path = getErrorCodeRerturnFile(code); 
    if (!error_html_path.empty()) {
        error_html_path = root_path_ + error_html_path; 
        struct stat file_stat {}; 
        if (stat(error_html_path.c_str(), &file_stat) == 0 &&
            faccessat(AT_FDCWD, error_html_path.c_str(), R_OK, AT_EACCESS) == 0) 
        { 
            HttpMethod method = getHttpMethodAsEnum(parser_.getMethod()); 
            if (method == HttpMethod::HEAD) {
                response.setBody(nullptr, file_stat.st_size);
                response.addHeader("Content-Type", "text/html"); 
            } else {
                if (mmap_data.mmap(error_html_path)) {
                    response.setBody(mmap_data.addr_, mmap_data.size_);
                    response.addHeader("Content-Type", "text/html"); 
                }
            }
        }
    }
    string version = parser_.getVersion(); 
    response.setVersion(version.empty() ? "HTTP/1.1" : version); // 若HTTP请求错误, 则解析结果无版本号.
    response.setStatus(code); 
    response.addHeader("Content-Length", to_string(response.getBodySize()));
    if (!parser_.isKeepAlive() || code == HttpStatusCode::BadRequest) {
        response.addHeader("Connection", "close"); 
    }
    LOG_TRACE << response.encode(); 
    sendResponse(response);
}


void HttpConnection::sendResponse(const HttpResponse& response) {
    if (auto tcp_conn_sptr = tcp_conn_wkptr.lock()) {
        // Buffer buf;  
        // response.encode(&buf); 
        // tcp_conn_sptr->send(&buf);
        struct iovec vecs[IOV_MAX]; 
        size_t iovcnt = response.encode(vecs, IOV_MAX);
        tcp_conn_sptr->send(vecs, iovcnt);
    }
}

void HttpConnection::onWriteComplete() {
    // 短连接下, 服务器端回发完响应报文后关闭写端.
    if (!parser_.isKeepAlive() || !parser_.parsingCompletion()) {
        shutdown();
        forceClose();
    }
    restartTimer(); 
}


bool MmapData::mmap(const string& file_path) {
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
    close(fd_); 
    fd_ = -1;
    return true;
}

MmapData::~MmapData() {
    if (addr_) {
        munmap(addr_, size_);
    }
    if (fd_ != -1) {
        close(fd_);
    }
}