#include "http_connection.h"
#include "http_message.h"

const unordered_map<HttpStatusCode, string> ERROR_RET_FILE_PATH = {
    { HttpStatusCode::BadRequest, "/400.html" },
    { HttpStatusCode::Forbidden, "/403.html" },
    { HttpStatusCode::NotFound, "/404.html"},
    { HttpStatusCode::InternalServerError, "/500.html"},
};

string getErrorCodeRerturnFile(HttpStatusCode code) {
    auto it =  ERROR_RET_FILE_PATH.find(code);
    if (it == ERROR_RET_FILE_PATH.end()) {
        return ""; 
    }
    return it->second; 
}

HttpConnection::HttpConnection(std::weak_ptr<TcpConnection> tcp_conn,
                               std::string root_path_, 
                               Duration timeout_duration)
    : timeout_duration_(timeout_duration),
    root_path_(root_path_),
    tcp_conn_wkptr(tcp_conn)
{
    auto conn_sptr = tcp_conn_wkptr.lock(); 
    if (conn_sptr) {
        LOG_TRACE << "HttpConnection: add Timer";
        timer_wkptr_ = conn_sptr->getOwnerLoop()->runAfter(
        timeout_duration_, bind(&HttpConnection::forceClose, this));
    }
}


void HttpConnection::restartTimer() {
    auto conn_sptr = tcp_conn_wkptr.lock(); 
    auto timer_sptr = timer_wkptr_.lock();
    // conn或timer不存在, 均意味着TCP连接已断开. timer的定时任务即为forceClose掉Tcp连接.
    if (conn_sptr && timer_sptr) {
        LOG_TRACE << "HttpConnection::restartTimer";
        conn_sptr->getOwnerLoop()->removeTimer(timer_wkptr_); 
        timer_wkptr_ = conn_sptr->getOwnerLoop()->runAfter(
            timeout_duration_, bind(&HttpConnection::forceClose, this));
    }
}

void HttpConnection::shutdown() const {
    auto conn_sptr = tcp_conn_wkptr.lock(); 
    if (conn_sptr) {
        conn_sptr->shutdown(); 
    }
}

void HttpConnection::forceClose() const {
    auto conn_sptr = tcp_conn_wkptr.lock(); 
    auto timer_sptr = timer_wkptr_.lock();
    if (conn_sptr && timer_sptr) {
        // 清除定时器, 关闭tcp连接. 
        conn_sptr->getOwnerLoop()->removeTimer(timer_wkptr_); 
        conn_sptr->forceClose(); 
    }
}


void HttpConnection::handleMessage(Buffer* buf) {
    // 重置超时时间
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
        sendErrorResponse(HttpStatusCode::BadRequest);
        // tcp_conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
        // tcp_conn->shutdown();
    } else {
        LOG_TRACE << "HttpServer::onMessage http request parsing continue"; 
    }
}

void HttpConnection::handleRequest() {
    LOG_INFO << parser_.encode(); 
    HttpResponse response; 
    HttpStatusCode code = HttpStatusCode::OK; 
    MmapData mmap_data; 

    // 处理url路径, 若有效且目标文件存在&可访问, 则path设置为文件的绝对路径.
    string path = parser_.getUri(); 
    code = processPath(path);  
    if (code != HttpStatusCode::OK) {
        sendErrorResponse(code);
        return; 
    }

    // TODO: 根据文件后缀填充字段类型(Content-Type)
    
    // 区分HEAD与GET, GET不处理
    if (code == HttpStatusCode::OK && !strncasecmp(parser_.getMethod(), "GET", 3)) {
        // 请求的资源文件由mmap映射到进程内存中.
        bool ret = mmap_data.mmap(path); 
        if (ret) {
            response.setBody(mmap_data.addr_, mmap_data.size_);  // 自动添加"Content-Length"头部
        } else {
            sendErrorResponse(HttpStatusCode::InternalServerError);
            return;
        }
    }
    
    // 构造响应报文
    response.setVersion(parser_.getVersion());
    response.setStatus(code);
    response.addHeader("Content-Length", to_string(response.getBodySize()));   // 自动填充Content-Length头
    if (!parser_.getKeepAlive()) {
        response.addHeader("Connection", "close"); 
    }
    sendResponse(response); 
}


void HttpConnection::sendErrorResponse(const HttpStatusCode& code) {
    HttpResponse response; 
    MmapData mmap_data;
    response.setVersion(parser_.getVersion());
    response.setStatus(code);

    // 根据错误状态码, 获取对应返回的HTML文件: 
    string error_html_path = getErrorCodeRerturnFile(code); 
    if (!error_html_path.empty()) {
        error_html_path = root_path_ + error_html_path; 
        if (mmap_data.mmap(error_html_path)) {
            response.setBody(mmap_data.addr_, mmap_data.size_);
        }
    } 

    response.addHeader("Content-Length", to_string(response.getBodySize()));   // 自动填充Content-Length头
    if (!parser_.getKeepAlive()) {
        response.addHeader("Connection", "close"); 
    }
    sendResponse(response);
}



void HttpConnection::sendResponse(const HttpResponse& response) {
    // 临时用栈空间作为Http的应用层缓冲区, 只存放请求行+头部.
    if (auto tcp_conn_sptr = tcp_conn_wkptr.lock()) {
        Buffer buf;  
        response.encode(&buf); 
        tcp_conn_sptr->send(&buf);
    }
    // 短连接下, 服务器端回发响应报文后关闭写端.
    if (!parser_.getKeepAlive()) {
        shutdown();
    }
    // 服务器内部错误, 考虑关闭整个连接?
    string code_str = response.getCodeAsString(); 
    if (code_str[0] == '5') {
        forceClose();
    }
    // 重置超时时间
    restartTimer(); 
}

// 处理url路径, 若有效且目标文件存在&可访问, 则返回文件的绝对路径.
HttpStatusCode HttpConnection::processPath(string& path) {
    // FIXME: 不应该先加上root再解析, 而是解析完再加上root, 由此确保始终只在资源目录下.
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



bool HttpConnection::MmapData::mmap(const string& file_path) {
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

HttpConnection::MmapData::~MmapData() {
    if (addr_) {
        munmap(addr_, size_);
    }
    if (fd_ != -1) {
        close(fd_);
    }
}