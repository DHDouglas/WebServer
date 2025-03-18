#include "http_message.h"

#include <cassert>
#include <cstring>
#include <strings.h>

#include "logger.h"

using namespace std;


HttpMethod getHttpMethodAsEnum(const std::string &method) {
    if (strncasecmp(method.c_str(), "HEAD", 4) == 0) {
        return HttpMethod::HEAD;
    } else if (strncasecmp(method.c_str(), "GET", 3) == 0) {
        return HttpMethod::GET;
    } else if (strncasecmp(method.c_str(), "POST", 4) == 0) {
        return HttpMethod::POST;
    }
    return HttpMethod::UNKNOWN;
}

std::string  getHttpMethodAsString(const HttpMethod& method) {
    switch (method) {
        case HttpMethod::GET: return "GET"; 
        case HttpMethod::HEAD: return "HEAD";
        default: return "UNKNOWN"; 
    }
}

string getHttpStatusCodeString(HttpStatusCode code) {
    auto it =  STATUS_CODE_PHASE.find(code);
    if (it == STATUS_CODE_PHASE.end()) {
        return ""; 
    }
    return it->second.first; 
}

string getHttpStatusCodePhaseString(HttpStatusCode code) {
    auto it =  STATUS_CODE_PHASE.find(code);
    if (it == STATUS_CODE_PHASE.end()) {
        return ""; 
    }
    return it->second.first + " " + it->second.second;  
}


string getContentType(const std::string& path) {
    string::size_type pos = path.find_last_of('.'); 
    if (pos == string::npos) {
        return "text/plain";
    }
    string suffix = path.substr(pos);
    auto it = CONTENT_TYPE.find(suffix); 
    if (it != CONTENT_TYPE.end()) {
        return it->second;
    }
    return "text/plain";
}

bool HttpMessage::setVersion(const string str) {
    if (str == "HTTP/1.1" || str == "HTTP/1.0" || str == "HTTP/0") {
        version_ = std::move(str);  
        return true;
    }
    return false; 
}


bool HttpMessage::addHeader(const string name, const string value) {
    if (name.length() == 0 || value.length() == 0) return false; 
    headers_.emplace_back(std::move(name), std::move(value)); 
    return true;
}


bool HttpMessage::setHeader(const string name, const string value) {
    for (auto& h : headers_) {
        if (h.first.length() == name.length() && strcasecmp(h.first.c_str(), name.c_str()) == 0) {
            h.second = std::move(value); 
            return true; 
        }
    }
    return addHeader(std::move(name), std::move(value)); 
}


bool HttpMessage::setHeaders(vector<pair<string, string>> headers) {
    this->headers_ = std::move(headers); 
    return true;
}


bool HttpMessage::setBody(const void* data, size_t len) {
    // if (len > 0 && data != nullptr) {
    // 允许body为空, 处理HEAD请求, 需要获取大小, 但不需要返回body.
    body_ = data; 
    body_size_ = len; 
    // TODO: if chunked
    return true;
}

int HttpMessage::encode(struct iovec vectors[], int max) const{
    fillStartLine();
    assert(max >= 6); 
    vectors[0].iov_base = (void*)startline_[0].base;  // static_cast<void*>(const_cast<char*>(version.c_str())); 
    vectors[0].iov_len = startline_[0].len; 
    vectors[1].iov_base = (void*)" ";  // static_cast<void*>(const_cast<char*>(" ")); 
    vectors[1].iov_len = 1; 
    vectors[2].iov_base = (void*)startline_[1].base; 
    vectors[2].iov_len = startline_[1].len; 
    vectors[3].iov_base = (void*)" ";
    vectors[3].iov_len = 1;
    vectors[4].iov_base = (void*)startline_[2].base;
    vectors[4].iov_len = startline_[2].len;   
    vectors[5].iov_base = (void*)"\r\n"; 
    vectors[5].iov_len = 2; 

    int i = 6; 
    for (auto& t : headers_) {
        if (i + 4 > max) {
            errno = EOVERFLOW;  
            return -1; 
        }
        vectors[i].iov_base = (void*)t.first.data();   // name
        vectors[i].iov_len = t.first.length(); 
        ++i; 
        vectors[i].iov_base = (void*)": "; 
        vectors[i].iov_len = 2;
        ++i; 
        vectors[i].iov_base = (void*)t.second.data();  // value
        vectors[i].iov_len = t.second.length(); 
        ++i; 
        vectors[i].iov_base = (void*)"\r\n";
        vectors[i].iov_len = 2; 
        ++i;
    }    

    if (i + 2 > max) {
        errno = EOVERFLOW; 
        return -1; 
    }
    vectors[i].iov_base = (void*)"\r\n";
    vectors[i].iov_len = 2; 
    ++i;

    if (body_ && body_size_ > 0) {
        if (i == max) {
            errno = EOVERFLOW;
            return -1; 
        }
        vectors[i].iov_base = (void*)body_; 
        vectors[i].iov_len = body_size_;   
        ++i;
    }
    return i; 
}

void HttpMessage::encode(Buffer* buf) const {
    fillStartLine();  
    buf->append(startline_[0].base, startline_[0].len); 
    buf->append(" ");
    buf->append(startline_[1].base, startline_[1].len);
    buf->append(" ");
    buf->append(startline_[2].base, startline_[2].len);
    buf->append("\r\n"); 
    for (auto& t : headers_) {
        buf->append(t.first.data(), t.first.length()); 
        buf->append(": ");
        buf->append(t.second.data(), t.second.length());
        buf->append("\r\n"); 
    }
    buf->append("\r\n");
    if (body_ && body_size_ > 0) {
        LOG_TRACE << " HttpMessage::encode buf->append(body, body_size), body: " 
                  << body_ << "body_size: " << body_size_; 
        buf->append(body_, body_size_);
    }
}


string HttpMessage::encode() const {
    fillStartLine(); 
    string ret = string(startline_[0].base, startline_[0].len);
    ret += " ";
    ret += string(startline_[1].base, startline_[1].len);
    ret += " ";
    ret += string(startline_[2].base, startline_[2].len);
    ret += "\r\n";
    for (auto& t: headers_) {
        ret += t.first;
        ret += ": ";
        ret += t.second;
        ret += "\r\n";
    }
    ret += "\r\n";
    if (body_ && body_size_ > 0) {
        ret += string((char*)body_, body_size_); 
    }
    return ret;
}

// ----------------------------------------HttpRequest begin

bool HttpRequest::setMethod(HttpMethod method) {
    switch(method) {
        case HttpMethod::GET: 
            this->method_ = "GET"; break;

        case HttpMethod::HEAD: 
            this->method_ = "HEAD"; break; 
        
        case HttpMethod::POST: 
            this->method_ = "POST"; break;
        
        default: 
            return false; 
    }
    return true;
}

bool HttpRequest::setMethod(const string str) {
    if (str == "GET" || str == "HEAD" || str == "POST") {
        this->method_ = str;
        return true; 
    }
    return false; 
}

bool HttpRequest::setUri(const string str) {
    this->uri_ = std::move(str);
    return true;
}

void HttpRequest::fillStartLine() const {
    startline_[0].base = method_.data(); 
    startline_[0].len = method_.length();
    startline_[1].base = uri_.data();
    startline_[1].len = uri_.length();
    startline_[2].base = version_.data();
    startline_[2].len = version_.length(); 
    assert(startline_[0].base);
    assert(startline_[0].len > 0);
    assert(startline_[1].base);
    assert(startline_[1].len > 0); 
    assert(startline_[2].base);
    assert(startline_[2].len > 0); 
}

// ----------------------------------------HttpRequest end




// ----------------------------------------HttpResponse begin

bool HttpResponse::setStatus(HttpStatusCode code) {
    auto p = STATUS_CODE_PHASE.find(code);
    code_ = code;
    if (p != STATUS_CODE_PHASE.end()) {
        this->code_str_ = p->second.first;
        this->phase_ = p->second.second; 
        return true; 
    }
    return false; 
}

void HttpResponse::fillStartLine() const {
    startline_[0].base = version_.data(); 
    startline_[0].len = version_.length();
    startline_[1].base = code_str_.data();
    startline_[1].len = code_str_.length();
    startline_[2].base = phase_.data();
    startline_[2].len = phase_.length(); 
    assert(startline_[0].base);
    assert(startline_[0].len > 0);
    assert(startline_[1].base);
    assert(startline_[1].len > 0); 
    assert(startline_[2].base);
    assert(startline_[2].len > 0); 
}

// ----------------------------------------HttpResponse end