#include "http_parser.h"

#include <cassert>
#include <cstring>

using namespace std; 

HttpParser::HttpParser() {
    reset(); 
}

void HttpParser::reset() {
    parse_phase = ParsePhase::REQUEST_LINE;
    parse_rl_state = ParseRequestLineState::BEFORE_METHOD; 
    parse_hd_state = ParseHeaderState::BEFORE_KEY;
    content_length = 0;
    transfer_length = 0;  
    header_offset = 0; 
    body_offset = 0; 
    name_buf_pos = 0; 
    keep_alive = false;
    chunked = false; 
    pos = nullptr;
    end = nullptr; 
    headers.clear(); 
    body.clear();  
}


HttpParser::ParseResult HttpParser::parse(const char* data, const size_t len, size_t& parsed_size) {
    if (parse_phase == ParsePhase::DONE) {
        reset(); 
    }
    this->pos = data;
    this->end = data + len; 
    size_t msg_size = header_offset + body_offset;  // 记录当前已解析得到的消息字节数. 

    ParseResult ret = ParseResult::SUCCESS; 
    while (ret == ParseResult::SUCCESS && parse_phase != ParsePhase::DONE) {
        switch (parse_phase) {
            case ParsePhase::REQUEST_LINE: {
                ret = parseRequestLine(); 
                if (ret == ParseResult::SUCCESS) {
                    parse_phase = ParsePhase::HEADER; 
                }
            }; break;

            case ParsePhase::HEADER: { 
                ret = parseHeader(); 
                if (ret == ParseResult::SUCCESS) {
                    if (this->content_length > 0 || this->chunked) {  // POST且content-length非0, 或者存在chunk
                        parse_phase = ParsePhase::BODY;
                    } else {
                        parse_phase = ParsePhase::DONE; 
                    }
                }
            } break; 

            case ParsePhase::BODY: {
                ret = parseBody(); 
                if (ret == ParseResult::SUCCESS) {
                    parse_phase = ParsePhase::DONE; 
                }
            } break; 

            case ParsePhase::DONE: break;
        }
    }

    // 计算此次调用中解析得到的有效字节数.
    assert(parsed_size <= len); 
    parsed_size = header_offset + body_offset - msg_size; 
    return ret; 
};


HttpParser::ParseResult HttpParser::parseRequestLine() {
    using State = ParseRequestLineState; 
    const char* method, *uri, *version; 
    while (pos != end) {
        switch (parse_rl_state) {
            case State::BEFORE_METHOD: {
                method = pos; 
                parse_rl_state = State::METHOD; 
            } break; 

            case State::METHOD: {
                if (*pos == ' ') {
                    parse_rl_state = State::BEFORE_URI; 
                    if (!checkMethod(method, pos - method)) {
                        return ParseResult::ERROR; 
                    }
                    header_offset += pos - method + 1;    // 包括空格. 
                } 
                ++pos; 
            } break; 

            case State::BEFORE_URI: {
                uri = pos; 
                parse_rl_state = State::URI; 
            } break; 
            
            case State::URI: {
                if (*pos == ' ') {
                    parse_rl_state = State::BEFORE_VERSION; 
                    if (!checkUri(uri, pos - uri)) {
                        return ParseResult::ERROR; 
                    }
                    header_offset += pos - uri + 1;
                }
                ++pos; 
            } break;

            case State::BEFORE_VERSION: {
                version = pos; 
                parse_rl_state = State::VERSION; 
            } break; 

            case State::VERSION: {
                if (*pos == '\r') {
                    parse_rl_state = State::LF; 
                    if (!checkVersion(version, pos - version)) {
                        return ParseResult::ERROR; 
                    }
                    header_offset += pos - version + 1; 
                }
                ++pos; 
            } break; 

            case State::LF: {
                if (*pos == '\n') {
                    ++header_offset; 
                    ++pos; 
                    return ParseResult::SUCCESS; 
                } else {
                    return ParseResult::ERROR; 
                }
            } break;    
        }
    }
    
    // 到这里, 若某一字段解析未完成, 需要对状态进行回退. 
    switch (parse_rl_state) {
        case State::METHOD: 
            parse_rl_state = State::BEFORE_METHOD; break; 
        case State::URI:
            parse_rl_state = State::BEFORE_URI; break;
        case State::VERSION:
            parse_rl_state = State::BEFORE_VERSION; break;

        case State::BEFORE_METHOD:
        case State::BEFORE_URI:
        case State::BEFORE_VERSION: 
        case State::LF: break; 
    }
    return ParseResult::AGAIN; 
}


HttpParser::ParseResult HttpParser::parseHeader() {
    // 解析name时利用一个name_buf缓冲区记录name, 而value则直接用指针指示起始位置. 
    // 当得到完整的name-value后, 再记录. 
    using State = ParseHeaderState;
    const char* value; 
    while (pos != end) { 
        switch (parse_hd_state) {
            case State::BEFORE_KEY: {
                if (*pos == '\r') {
                    parse_hd_state = State::END_CR;
                } else {
                    name_buf_pos = 0; 
                    parse_hd_state = State::KEY; 
                }
            } break;

            case State::KEY: {
                if (*pos == ':') {
                    name_buf[name_buf_pos++] = '\0'; 
                    parse_hd_state = State::SPACE; 
                    header_offset += name_buf_pos;   // 包括':'
                } else {
                    name_buf[name_buf_pos++] = *pos; 
                    if (name_buf_pos == HEADER_NAME_MAX) {
                        return ParseResult::ERROR; 
                    }
                }
                ++pos;  
            } break; 

            case State::SPACE: {
                if (*pos++ == ' ') {
                    ++header_offset; 
                    parse_hd_state = State::BEFORE_VALUE; 
                } else {
                    return ParseResult::ERROR; 
                }
            } break;

            case State::BEFORE_VALUE: {
                value = pos; 
                parse_hd_state = State::VALUE; 
            } break; 

            case State::VALUE: {
                if (*pos == '\r') {
                    if (!checkHeader(name_buf, name_buf_pos - 1, value, pos - value)) {   // name_buf_pos-1, 不计入'\0'. 
                        return ParseResult::ERROR; 
                    }
                    parse_hd_state = State::LF; 
                    header_offset += pos - value + 1;  // 包括'\r'
                }
                ++pos; 
            } break; 

            case State::LF: {
                if (*pos++ != '\n') {
                    return ParseResult::ERROR; 
                }
                ++header_offset;
                parse_hd_state = State::END_CR; 
            } break; 

            case State::END_CR: {
                if (*pos == '\r') {  
                    ++header_offset;
                    parse_hd_state = State::END_LF; 
                    ++pos; 
                } else {  // 新一个头部
                    parse_hd_state = State::BEFORE_KEY; 
                }
            } break;

            case State::END_LF: {
                if (*pos++ != '\n') {
                    return ParseResult::ERROR;
                }
                ++header_offset;
                return ParseResult::SUCCESS; 
            } break;
        }
    }

    if (parse_hd_state == State::KEY) {
        parse_hd_state = State::BEFORE_KEY; 
    } else if (parse_hd_state == State::VALUE) {
        parse_hd_state = State::BEFORE_VALUE; 
    }

    return ParseResult::AGAIN;
}


HttpParser::ParseResult HttpParser::parseBody() {
    // 剩余长度: 
    size_t len = end - pos; 
    if (this->chunked) {
        return ParseResult::SUCCESS; 
    } else {
        body.reserve(content_length);   // 保留足够的content_length空间. 
        if (body_offset + len >= content_length) {  // 剩余内容覆盖了完整body. 
            body.insert(body.end(), pos, pos + content_length - body_offset);  
            body_offset = content_length;  
            return ParseResult::SUCCESS; 
        } else {
            body.insert(body.end(), pos, pos + len); 
            body_offset += len; 
            return ParseResult::AGAIN; 
        }
    }
}


bool HttpParser::checkMethod(const char* str, size_t len) {
    bool ret = false; 
    if (strncasecmp(str, "GET",  len) == 0) ret = true; 
    if (strncasecmp(str, "POST", len) == 0) ret = true;
    if (strncasecmp(str, "HEAD", len) == 0) ret = true;
    if (ret) {
        this->method = string(str, len); 
    }
    return ret;
}

bool HttpParser::checkUri(const char* str, size_t len) {
    this->uri = string(str, len); 
    return true; 
}

bool HttpParser::checkVersion(const char* str, size_t len) {
    if (strncasecmp(str, "HTTP/1.0", len) == 0 || strncasecmp(str, "HTTP/0", len) == 0) {
        this->version = string(str, len); 
        this->keep_alive = false; 
        return true;
    } else if (strncasecmp(str, "HTTP/1.1", len) == 0) {
        this->version = string(str, len); 
        this->keep_alive = true;   
        return true;
    }
    return false;
}


bool HttpParser::checkHeader(const char* name, size_t n_len, const char* value, size_t v_len) {
    switch (n_len) {
        case 10: {
            if (strncasecmp(name, "Connection", 10) == 0) {
                this->has_connection = true;
                if (v_len == 10 && strncasecmp(value, "Keep-Alive", 10) == 0) {
                    this->keep_alive = true;
                } else if (v_len == 5 && strncasecmp(value, "close", 5) == 0) {
                    this->keep_alive = false; 
                }
            } else if (strncasecmp(name, "Keep-Alive", 10) == 0) {
                this->has_keep_alive = true; 
            }
        } break;

        case 14: {
            if (strncasecmp(name, "Content-Length", 14) == 0) {
                // this->has_content_length = true; 
                this->content_length = stoi(string(value, v_len)); 
            }
        } break;

        case 17: {
            if (strncasecmp(name, "Transfer-Encoding", 17) == 0 && strncasecmp(value, "chunked", 7) == 0) {
                this->chunked = true; 
            }
        } break; 
    }

    headers.emplace_back(string(name, n_len), string(value, v_len)); 
    return true; 
}


string HttpParser::encode() const {
    assert(parse_phase == ParsePhase::DONE);
    string ret = method + " " + uri + " " + version + "\r\n";
    for (auto& p : headers) {
        ret += p.first + ": " + p.second + "\r\n"; 
    }
    ret += "\r\n";
    ret += string(body.data(), body_offset); 
    return ret;
}


bool HttpParser::parsingCompletion() {
    return parse_phase == ParsePhase::DONE;
}

const char* HttpParser::getMethod() const {
    return method.data(); 
}

const char* HttpParser::getUri() const {
    return uri.data();
}

const char* HttpParser::getVersion() const {
    return version.data(); 
}

const char* HttpParser::getBody() const {
    return body.data(); 
}

size_t HttpParser::getBodySize() const {
    return body.size(); 
}

const vector<pair<string, string>>& HttpParser::getHeaders() const {
    return headers; 
}

bool HttpParser::getKeepAlive() const {
    return keep_alive; 
}