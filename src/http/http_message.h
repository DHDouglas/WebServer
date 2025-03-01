#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cassert>
#include <sys/uio.h>
#include <cstring>

using namespace std;

enum class HttpMethod {
    GET = 0,
    POST,
    HEAD,
}; 

enum class HttpStatusCode {
    Continue = 100,  
	OK = 200,
    BadRequest = 400,
    Forbidden = 403, 
    NotFound = 404, 
    InternalServerError = 500,
}; 


const std::unordered_map<HttpStatusCode, std::pair<std::string, std::string>> 
STATUS_CODE_PHASE {
    {HttpStatusCode::Continue,    {"100", "Continue"}},
    {HttpStatusCode::OK,          {"200", "OK"}}, 
    {HttpStatusCode::BadRequest,  {"400", "BadRequest"}},
    {HttpStatusCode::Forbidden,   {"403", "Forbidden"}},
    {HttpStatusCode::NotFound,    {"404", "NotFound"}},
    {HttpStatusCode::InternalServerError,  {"500", "InternalServerError"}},
}; 

const int ENCODE_IOV_MAX = 2048; 


class HttpMessage {
public:
    bool set_version(const std::string str); 
    bool add_header(const std::string name, const std::string value);
    bool set_header(const string name, const string value);
    bool set_headers(std::vector<std::pair<std::string, std::string>> headers); 
    bool set_body(const void* data, size_t len); 
    const void* get_body();
    size_t get_body_size();  
    int encode(struct iovec vectors[], int max);
    std::string encode(); 

protected:    
    HttpMessage(): body(nullptr), body_size(0) {};
    ~HttpMessage() = default; 

    struct field {
        const char* base;
        size_t len; 
    } startline[3]; 
    
    std::string version;
    std::vector<std::pair<std::string, std::string>> headers; 
    const void* body;    // 如何判断body是来自mmap, 还是一片动态内存? 
    size_t body_size; 
};


class HttpRequest: public HttpMessage {
public:
    HttpRequest() = default;
    ~HttpRequest() = default;
    bool set_method(const string str);
    bool set_method(HttpMethod method);
    bool set_uri(const std::string str); 
    int encode(struct iovec vectors[], int max);
    std::string encode(); 

private:
    std::string method;
    std::string uri;
};


class HttpResponse: public HttpMessage {
public:
    HttpResponse() = default;
    ~HttpResponse() = default; 
    bool set_status(HttpStatusCode status);  // 根据状态码, 自动设定值, 短语; 
    int encode(struct iovec vectors[], int max);
    std::string encode(); 

private:
    std::string code;
    std::string phase;
};
