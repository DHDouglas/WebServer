#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>

#include <iostream> 

#include "http_conn.h"
#include "http_message.h"
#include "http_parser.h"



const unordered_map<string, string> SUFFIX_CONTENT_TYPE {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
};



const char* kDefultErrorContentTemplate = 
    "<html><title>Error</title>"
    "<body bgcolor=\"ffffff\">"
    "%s : %s\n"
    "<p>%s</p>"
    "<hr><em>HttpServer</em></body></html>"; 



HttpConn::HttpConn() {
    m_sockfd = -1;
    m_addr = {0}; 
    m_is_close = true;  
}

HttpConn::~HttpConn() {


}


void HttpConn::init(int sockfd, const sockaddr_in& addr) {
    m_sockfd = sockfd;
    m_addr = addr; 
    // 初始化读写缓冲区
    m_is_close = false; 
    m_inputBuffer.RetrieveAll();
    m_outputBuffer.RetrieveAll();
}

ssize_t HttpConn::read(int* saveErrno) {
    ssize_t len = -1;
    len = m_inputBuffer.ReadFd(m_sockfd, saveErrno); 
    return len; 
}

// ssize_t HttpConn::write(int* saveErrno) {
//     ssize_t len = -1;
// }



const char* GetDefaultErrorContent(HttpStatusCode code, string msg) {
    size_t size = strlen(kDefultErrorContentTemplate) + msg.size() + 128;
    char* buf = new char[size]; 
    auto code_phase = STATUS_CODE_PHASE.find(code)->second;
    snprintf(buf, size, kDefultErrorContentTemplate, code_phase.first.c_str(), code_phase.second.c_str(), msg.c_str());  
    return buf; 
}




// HttpResponse HttpConn::get_response(HttpStatusCode code){
// }


// HttpResponse HttpConn::get_default_error_reponse() {
// }



// #if 0

// FIXME: 该过程即作为HttpTask中的回调函数, 实现服务器的业务逻辑. 
bool HttpConn::process() {
    // 涉及到哪些具体处理步骤? 
    // 1) parser.parse() 调用Parser进行解析
    // 2) 如果解析状态为ParseAGAIN, 就将设备描述符再次注册为监听EPOLLIN; 
    // 3) 如果解析状态为ParseSuccess:
    // - 获取parser中解析得到的内容, 拷贝到该HttpConn中; 
    // - 调用buff.Retrive, 表示已读取缓冲区中内容. => 需要获取结束标记位置. 
    // 4) 调用处理, 进行响应. =>  构造响应报文, 将响应报文写入HttpConn的"写缓冲区".   
    // 5)  将sockfd上的监听改为"EPOLLOUT", 等待写就绪. 
    // 6) sockfd上写事件就绪时, 调用系统调用, 将"写缓冲区"的内容写入fd, 发送给客户端. 

    // ----------------------
    // 1. 如果parser解析失败, 则状态码为400. 
    // 2. 如果方法是GET, 则检查请求文件, 如果请求文件不存在, 则状态码为404. 
    // 3. 如果方法是POST, 则获取消息体, 然后进行处理. 
    // 3. 根据"状态码", 头部属性如(keep-alive), 调用相应API添加头部信息. 

    // 获取解析得到的uri, 字段头等属性, 根据uri读取文件资源内容映射到内存中, 获取指向mmap的指针. 
    // deal_request(); 
    
    // 构造响应报文, 添加响应行, 添加头部, 添加content. 
    // 响应行, 头部从outputBuffer中读取, 而文件内容从mmap指针处读取. 
    // HttpResponse()

    // 针对GET请求, 检查uri对应的资源文件是否存在, 若存在打开该文件,     
    // 如果打开文件失败, 也要返回一个404
    // 对于一个GET请求, 例如/index.html. 如果不存在, 或者无权限访问, 则返回/404.html以及相应状态码. 
    // 如果/404.html也不存在, 则返回404状态码, 以及一个手动构造的html.  


    // TODO: Conn(实际上是HttpTask)应该具有的字段: 
    char* m_mmfile = nullptr; 
    void* m_file_address = nullptr; 
    struct stat m_mmfile_stat = {0}; 
    char m_srcdir[200];       // 资源根路径
    getcwd(m_srcdir, 200);    
    strcat(m_srcdir, "/resources"); 

    string filepath; 
    bool body_is_file = true; 
            

    HttpStatusCode code; 
    HttpParser::ParseResult ret; 

    
    size_t can_read = m_inputBuffer.ReadableBytes(); 
    size_t len =  can_read; 
    ret = parser.parse(m_inputBuffer.Peek(), len);  
    m_inputBuffer.Retrieve(len);     // 清除缓冲区中已被成功解析的数据. 


    if (ret == HttpParser::ParseResult::SUCCESS) {
        // FOR TEST: 
        // cout << parser.encode() << endl; 
        // const char* response = 
        //     "HTTP/1.1 200 OK\r\n"
        //     "Content-Length: 13\r\n"
        //     "Content-Type: text/plain\r\n"
        //     "\r\n"
        //     "Hello, World!";
        // send(m_sockfd, response, strlen(response), 0);  

        const char* method = parser.get_method(); 
        if (strcasecmp(method, "GET") == 0) {

            // TODO, 检查uri, 检查对应的资源是否存在. 
            // uri可能是 /video的形式, 这时候要取path+uri作为完整路径. 
            // 1) 拼接URI, 得到完整路径. 
            // 2) 检查资源是否存在. 若存在, 需要设置文件指针.  
            // 3) 设置状态码
            string url = parser.get_uri(); 
            if (url == "/") {
                filepath = "/index.html"; 
            } else if (
                url == "/index" || url == "/register" || url == "/login" || 
                url == "/welcome" || url == "/video" || url == "/picture") 
            {
                filepath = url + ".html";
            } else {
                filepath = url; 
            }

            filepath = m_srcdir + filepath; 
            if (stat(filepath.data(), &m_mmfile_stat) < 0 || S_ISDIR(m_mmfile_stat.st_mode)) {
                body_is_file = false; 
                code = HttpStatusCode::NotFound; 
            } else if (!(m_mmfile_stat.st_mode & S_IROTH)) {
                code = HttpStatusCode::Forbidden; 
            } else {
                code = HttpStatusCode::OK; 
            }

        } else if (strcasecmp(method, "POST")) {
            // TODO: 登录功能? 从Body中取出账号密码, 进行核验. 
            // UserVerify()
        } else if (strcasecmp(method, "HEAD")) {
            // TODO: 不返回Body, 只返回头部. 


        } else {
            // TODO: 未实现的方法
        }
    } else if (ret == HttpParser::ParseResult::AGAIN) {


    } else {  // Parse Error 
        // 返回 400. 
        code = HttpStatusCode::BadRequest; 
    }


    // TODO: error HTML
    if (code == HttpStatusCode::BadRequest) {
        filepath = string(m_srcdir) + "/400.html"; 
    } else if (code == HttpStatusCode::Forbidden) {
        filepath = string(m_srcdir) + "/403.html";
    } else if (code == HttpStatusCode::NotFound) {
        filepath = string(m_srcdir) + "/404.html"; 
    }

    if (code != HttpStatusCode::OK) {
        if (stat(filepath.data(), &m_mmfile_stat) < 0)  {
            // return ErrorContent(); 
            body_is_file = false; 
        }
    }

    if (body_is_file) {
        int fd = open(filepath.data(), O_RDONLY); 
        if (fd < 0) {
            // TODO: ERROR CONTENT
            // return ErrorContent(); 
            body_is_file = false; 
        }

        m_file_address = mmap(0, m_mmfile_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);  
        if (m_file_address == MAP_FAILED) {
            // ErrorContent(); 
            body_is_file = false; 
        }

    }

    // TODO: 根据状态码, 构造Response报文. 
    HttpResponse resp; 
    resp.set_version("HTTP/1.1"); 

    // 正常. 
    resp.set_status(code); 
    if (!parser.get_keep_alive()) {
        resp.set_header("Connection", "close"); 
    }

    // resp的body是const void*类型, 传递的实参是1) mmap的指针;  2) 构造的字符串; 
    if (body_is_file) {
        // 获取文件类型.  GetFileType_
        string filetype;
        string::size_type idx = filepath.find_last_of('.');
        if (idx == string::npos) {   
            filetype = "text/plain";  
        }
        string suffix = filepath.substr(idx);  
        if (SUFFIX_CONTENT_TYPE.count(suffix) == 1) {
            filetype = SUFFIX_CONTENT_TYPE.find(suffix)->second; 
        } else {
            filetype = "text/plain"; 
        }
        resp.add_header("Content-Type", filetype); 
        resp.set_body(m_file_address, m_mmfile_stat.st_size);  

    } else {
        resp.add_header("Content-Type", "text/html"); 
        const char* body = GetDefaultErrorContent(code, "File Not Found"); 
        resp.set_body(body, strlen(body));  
    }


    // TODO: 将resp内容写入到Output缓冲区. (必须, 否则返回后resp会被销毁, 要考虑body生命期). 
    // TODO: resp返回结构体, 调用Buffer的"const* void接口的Append写入, 不要用string, 会无法处理'\0'. 
    // m_outputBuffer.Append(resp.encode()); 
    // TODO: iovec to buf.
    struct iovec vectors[ENCODE_IOV_MAX]; 
    int cnt = resp.encode(vectors, ENCODE_IOV_MAX); 
    assert(cnt > 0 && cnt <= ENCODE_IOV_MAX); 

    for (int i = 0; i < cnt; ++i) {
        m_outputBuffer.Append(vectors[i].iov_base, vectors[i].iov_len); 
    }

    send(m_sockfd, m_outputBuffer.Peek(), m_outputBuffer.ReadableBytes(), 0);  

    // TODO:  手动释放body的指针, unmap或者delete[].
    // TODO: resp不知道, const void* body是来自哪里, char*buf 或者mm_file_addres. 所以不能由resp去释放. 
    if (body_is_file) {
        munmap(m_file_address, m_mmfile_stat.st_size);  
        m_file_address = nullptr;
    } else {
        const char* buf = static_cast<const char*>(resp.get_body()); 
        delete[] buf; 
    }
    return true;
}

// #endif


// string HttpConn::get_file_type() {
// }





bool HttpConn::deal_request() {
    // TODO:
    // 提取uri, 得到 "/...." 
    // 进行匹配, 拼接得到完整的资源文件地址. 
    // 打开文件, 将文件内容通过mmap映射到内存中, 内存地址返回给m_file_address. 
    // int fd = open(m_real_file, O_REONLY); 
    // m_file_address = (char *)mmap(0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0); 
    // close(fd);  
    return true; 
}
