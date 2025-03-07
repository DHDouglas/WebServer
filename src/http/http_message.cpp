#include "http_message.h"

using namespace std;

bool HttpMessage::set_version(const string str) {
    if (str == "HTTP/1.1" || str == "HTTP/1.0" || str == "HTTP/0") {
        version = std::move(str);  
        return true;
    }
    return false; 
}


bool HttpMessage::add_header(const string name, const string value) {
    if (name.length() == 0 || value.length() == 0) return false; 
    headers.emplace_back(std::move(name), std::move(value)); 
    return true;
}


bool HttpMessage::set_header(const string name, const string value) {
    for (auto& h : headers) {
        if (h.first.length() == name.length() && strcasecmp(h.first.c_str(), name.c_str()) == 0) {
            h.second = std::move(value); 
            return true; 
        }
    }
    return add_header(std::move(name), std::move(value)); 
}


bool HttpMessage::set_headers(vector<pair<string, string>> headers) {
    this->headers = std::move(headers); 
    return true;
}


bool HttpMessage::set_body(const void* data, size_t len) {
    if (len > 0 && data != nullptr) {
        body = data; 
        body_size = len; 
        set_header("Content-Length", to_string(body_size));   // 自动填充Content-Length头
        // TODO: if chunked
        return true;
    }
    return false;
}

const void* HttpMessage::get_body() {
    return body;
}

size_t HttpMessage::get_body_size() {
    return body_size; 
}

int HttpMessage::encode(struct iovec vectors[], int max) {
    assert(max >= 6); 
    vectors[0].iov_base = (void*)startline[0].base;  // static_cast<void*>(const_cast<char*>(version.c_str())); 
    vectors[0].iov_len = startline[0].len; 
    vectors[1].iov_base = (void*)" ";  // static_cast<void*>(const_cast<char*>(" ")); 
    vectors[1].iov_len = 1; 
    vectors[2].iov_base = (void*)startline[1].base; 
    vectors[2].iov_len = startline[1].len; 
    vectors[3].iov_base = (void*)" ";
    vectors[3].iov_len = 1;
    vectors[4].iov_base = (void*)startline[2].base;
    vectors[4].iov_len = startline[2].len;   
    vectors[5].iov_base = (void*)"\r\n"; 
    vectors[5].iov_len = 2; 

    int i = 6; 
    for (auto& t : headers) {
        if (i + 4 > max) {
            errno = EOVERFLOW;  
            return false; 
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
        return false; 
    }
    vectors[i].iov_base = (void*)"\r\n";
    vectors[i].iov_len = 2; 
    ++i;

    if (body_size > 0) {
        if (i == max) {
            errno = EOVERFLOW;
            return false; 
        }
        vectors[i].iov_base = (void*)body; 
        vectors[i].iov_len = body_size;   
    }
    return i + 1; 
}


string HttpMessage::encode() {
    string ret;
    for (auto& t: headers) {
        ret += t.first;
        ret += ": ";
        ret += t.second;
        ret += "\r\n";
    }
    ret += "\r\n";
    ret += string((char*)body, body_size); 
    return ret;
}

// --------------------HttpRequest
bool HttpRequest::set_method(HttpMethod method) {
    switch(method) {
        case HttpMethod::GET: 
            this->method = "GET"; break;

        case HttpMethod::HEAD: 
            this->method = "HEAD"; break; 
        
        case HttpMethod::POST: 
            this->method = "POST"; break;
        
        default: 
            return false; 
    }
    return true;
}


bool HttpRequest::set_method(const string str) {
    if (str == "GET" || str == "HEAD" || str == "POST") {
        this->method = str;
        return true; 
    }
    return false; 
}


bool HttpRequest::set_uri(const string str) {
    this->uri = std::move(str);
    return true;
}


int HttpRequest::encode(struct iovec vectors[], int max) {
    startline[0].base = method.data(); 
    startline[0].len = method.length();
    startline[1].base = uri.data();
    startline[1].len = uri.length();
    startline[2].base = version.data();
    startline[2].len = version.length(); 
    return HttpMessage::encode(vectors, max); 
}


string HttpRequest::encode() {
    string ret = method + " " + uri + " " + version + "\r\n"; 
    return ret + HttpMessage::encode();
}


// --------------------HttpResponse
bool HttpResponse::set_status(HttpStatusCode code) {
    auto p = STATUS_CODE_PHASE.find(code);
    if (p != STATUS_CODE_PHASE.end()) {
        this->code = p->second.first;
        this->phase = p->second.second; 
        return true; 
    }
    return false; 
}


int HttpResponse::encode(struct iovec vectors[], int max) {
    startline[0].base = version.data(); 
    startline[0].len = version.length();
    startline[1].base = code.data();
    startline[1].len = code.length();
    startline[2].base = phase.data();
    startline[2].len = phase.length(); 
    return HttpMessage::encode(vectors, max); 
}


string HttpResponse::encode() {
    string ret = version + " " + code + " " + phase + "\r\n"; 
    return ret + HttpMessage::encode();
}

