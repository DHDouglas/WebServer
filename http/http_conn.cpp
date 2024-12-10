#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/types.h>

#include "http_conn.h"



enum class HttpStatusCode {
    Continue = 100,  

	OK = 200, // RFC 7231, 6.3.1

    BadRequest = 400,
    NotFound = 404, 

    InternalServerError = 500,
}; 




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
}

ssize_t HttpConn::read(int* saveErrno) {
    ssize_t len = -1;
    len = m_inputBuffer.ReadFd(m_sockfd, saveErrno); 
    return len; 
}

// ssize_t HttpConn::write(int* saveErrno) {
//     ssize_t len = -1;
// }


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

    // parser.reset(); 
    // // FIXME: 解析后, 需要移动buffer的readpos. 
    // if (parser.parse(m_inputBuffer.Peek(), m_inputBuffer.ReadableBytes()) == ParseResult::SUCCESS) {
    //     // 测试. 
    //     parser.inspect(); 
    //     const char* response = 
    //         "HTTP/1.1 200 OK\r\n"
    //         "Content-Length: 13\r\n"
    //         "Content-Type: text/plain\r\n"
    //         "\r\n"
    //         "Hello, World!";
    //     send(m_sockfd, response, strlen(response), 0);  
    // }
    HttpStatusCode code;

    parser.reset(); 
    ParseResult res = parser.parse(m_inputBuffer.Peek(), m_inputBuffer.ReadableBytes()); 


    if (res == ParseResult::SUCCESS) {
        code = HttpStatusCode::OK; 
        parser.inspect(); 
        const char* response = 
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 13\r\n"
            "Content-Type: text/plain\r\n"
            "\r\n"
            "Hello, World!";
        send(m_sockfd, response, strlen(response), 0);  
    } else {
        code = HttpStatusCode::BadRequest; 
        // 返回
    }






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



    // 判断请求的资源文件. 
    // 以下为make response内容: 将HttpResponse实例中暂存的数据, 挨个写入到缓冲区中. 
    // add_http_version();
    // add_status_code();
    // add_reason_phrase(); 
    // // for 所有header. 
    // add_header_pair(); 
    // // 添加消息体. 
    // add_output_body();


    return true;
}


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
