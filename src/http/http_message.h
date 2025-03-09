#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cassert>
#include <sys/uio.h>
#include <cstring>

#include "buffer.h"


enum class HttpMethod {
    GET = 0,
    HEAD,
    POST,
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

std::string getHttpStatusCodeString(HttpStatusCode code);


class HttpMessage {
public:
    HttpMessage(): body_(nullptr), body_size_(0) {};
    virtual ~HttpMessage() = default; 
    bool setVersion(const std::string str); 
    bool addHeader(const std::string name, const std::string value);
    bool setHeader(const std::string name, const std::string value);
    bool setHeaders(std::vector<std::pair<std::string, std::string>> headers); 
    bool setBody(const void* data, size_t len); 
    const void* getBody() const { return body_; }
    size_t getBodySize() const { return body_size_; }

    int encode(struct iovec vectors[], int max) const;
    std::string encode() const; 
    void encode(Buffer* buffer) const;

protected:
    virtual void fillStartLine() const = 0;    // 纯虚函数

public:
    static const int ENCODE_IOV_MAX = 2048; 

protected:
    // 用于作为请求行/响应行中的三个字段的占位符.
    struct field {
        const char* base;
        size_t len; 
    } mutable startline_[3]; 
    
    std::string version_;
    std::vector<std::pair<std::string, std::string>> headers_; 
    const void* body_;    
    size_t body_size_; 
};


class HttpRequest: public HttpMessage {
public:
    HttpRequest() = default;
    ~HttpRequest() = default;
    bool setMethod(const std::string str);
    bool setMethod(HttpMethod method);
    bool setUri(const std::string str); 

private:
    void fillStartLine() const override; 
private:
    std::string method_;
    std::string uri_;
};


class HttpResponse: public HttpMessage {
public:
    HttpResponse() = default;
    ~HttpResponse() = default; 
    bool setStatus(HttpStatusCode code);  // 根据状态码, 自动设定值, 短语; 
    HttpStatusCode getCode() const  { return code_; }
    std::string getCodeAsString() const { return code_str_; }

private:
    void fillStartLine() const override; 

private:
    HttpStatusCode code_;
    std::string code_str_;
    std::string phase_;
};
