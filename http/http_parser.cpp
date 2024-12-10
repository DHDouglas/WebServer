#include "http_parser.h"



HttpParser::HttpParser() {
    reset(); 
}

void HttpParser::reset() {
    parse_result = ParseResult::AGAIN; 
    parse_phase = ParsePhase::REQUEST_LINE; 
    parse_rl_state = ParseRequestLineState::START; 
    parse_header_state = ParseHeaderState::START; 
    buf = nullptr; 
    buf_len = 0;  
    content_length = 0; 
    method = { nullptr, 0}; 
    uri = { nullptr, 0 };
    version = { nullptr, 0 };
    content = { nullptr, 0}; 
    headers.clear();  
}


ParseResult HttpParser::parse(const char* buf, size_t len) {
    // 如果传入buf与上一个buf相同, 表示继续解析; 否则重置Parser状态, 重新开始解析. 
    if (buf == this->buf) { 
        // 继续解析
        assert(len >= buf_len); 
        this->buf_len = len;   // 更新缓冲区长度, 继续解析.
        this->buf_end = buf + len; 
    } else {
        reset();   // 重置解析状态. 
        this->buf = buf;
        this->pos = buf; 
        this->buf_len = len; 
        this->buf_end = buf + len; 
    }

    // return parse_request_line(); 
    // return parse_header(); 

    while (pos != buf_end) {
        switch (parse_phase) {
            case ParsePhase::REQUEST_LINE:  {
                // 判断, 根据返回值, 若成功解析则转移到下一状态
                parse_result = parse_request_line(); 
                if (parse_result == ParseResult::SUCCESS) {
                    parse_phase = ParsePhase::HEADER; 
                } else {
                    return parse_result;   // 返回ERROR或AGAIN. 
                }
            } break;

            case ParsePhase::HEADER: {
                parse_result = parse_header(); 
                if (parse_result == ParseResult::SUCCESS) {      
                    if (strncasecmp(method.pos, "POST", method.len) == 0 && content_length > 0) {
                        parse_phase = ParsePhase::CONTENT; 
                    } else {
                        done = true; 
                        return parse_result ;  // 解析完成. 
                    }
                } else if (parse_result == ParseResult::ERROR) {
                    return parse_result;  
                }
            } break;

            case ParsePhase::CONTENT: {
                parse_result = parse_body(); 
                if (parse_result == ParseResult::SUCCESS) {
                    done = true;  // 解析完成. 
                    return ParseResult::SUCCESS;  
                }
            } break; 
        }
    }
    // 标记解析未完成, 等待下一次解析. 
    return ParseResult::AGAIN; 
}



ParseResult HttpParser::parse_request_line() { 
    using State = ParseRequestLineState; 

    while (true) {
        // 缓冲区: [buf, buf_end); 
        if (pos == buf_end) return ParseResult::AGAIN;  // 数据不完整
        const char ch = *pos++;      // 先后移pos, 保证下面return时, pos指向下一待处理位置. 
        switch(parse_rl_state) {
            case State::START: {
                parse_rl_state = State::METHOD; 
                method.pos = pos - 1; 
            } break; 

            case State::METHOD: {
                if (ch == ' ') {
                    parse_rl_state = State::BEFORE_URI;
                    method.len = pos - method.pos - 1;
                    if (!check_method(method)) {
                        return ParseResult::ERROR;
                    }
                }
            }; break; 

            case State::BEFORE_URI: {
                uri.pos = pos - 1;   // 记录uri起始位置. 
                parse_rl_state = State::URI;
            } break;

            case State::URI: {
                if (ch == ' ') {
                    parse_rl_state = State::BEFORE_VERSION;
                    uri.len = pos - uri.pos - 1;
                    if (!check_uri(uri)) {  // 判断url是否合法. 
                        return ParseResult::ERROR; 
                    };  
                } 
            } break; 

            case State::BEFORE_VERSION: {
                version.pos = pos - 1;  // 记录version起始位置. 
                parse_rl_state = State::VERSION; 
            }

            case State::VERSION: {
                if (ch == '\r') {
                    parse_rl_state = State::CR;
                    version.len = pos - version.pos - 1; 
                    if (!check_version(version)) {
                        return ParseResult::ERROR;
                    }
                }
            } break; 

            case State::CR: {
                if (ch == '\n') {  // 结束提取. 
                    return ParseResult::SUCCESS; 
                } else {
                    return ParseResult::ERROR;  
                }
            } break;
        }
    }
}


ParseResult HttpParser::parse_header() {
    using State = ParseHeaderState; 

    while (true) {
        if (pos == buf_end) return ParseResult::AGAIN; 
        const char ch = *pos++;   // pos points to next unprocessed ch. 
        switch (parse_header_state) {
            case State::START: {
                headers.emplace_back(pos - 1, 0, nullptr, 0);
                parse_header_state = State::KEY; 
            } break;

            case State::KEY: {
                if (ch == ':') {
                    headers.back().k_len = pos - 1 - headers.back().key; 
                    parse_header_state = State::COLON;  
                }
            } break;

            case State::COLON: {
                if (ch == ' ') {
                    parse_header_state = State::SPACE;
                } else {
                    return ParseResult::ERROR;  
                }
            } break;

            case State::SPACE: {
                headers.back().value = pos - 1; 
                parse_header_state = State::VALUE; 
            } break;

            case State::VALUE: {
                if (ch == '\r') {
                    parse_header_state = State::CR; 
                    headers.back().v_len = pos - 1 - headers.back().value;   
                    // 得到一个完整header. 
                    if (!check_header(headers.back())) {
                        return ParseResult::ERROR; 
                    }
                }
            } break;

            case State::CR: {
                if (ch == '\n') {
                    parse_header_state = State::LF; 
                } else {
                    return ParseResult::ERROR; 
                }
            } break;

            case State::LF: {
                if (ch == '\r') {
                    parse_header_state = State::END_CR; 
                } else { 
                    // 继续解析下一个头部. 
                    parse_header_state = State::KEY;  
                    headers.emplace_back(pos - 1, 0, nullptr, 0);
                } 
            } break;

            case State::END_CR: {
                if (ch == '\n') {
                    // 结束请求头识别
                    return ParseResult::SUCCESS; 
                } else {
                    return ParseResult::ERROR; 
                }
            } break; 
        }
    }
}


ParseResult HttpParser::parse_body() {
    if (buf_end - pos < content_length) {  // 等待接收完整body后才处理; 
        return ParseResult::AGAIN;
    } else {
        content.pos = pos; 
        content.len = content_length; 
        return ParseResult::SUCCESS; 
    }
}


bool HttpParser::check_method(Text& text) {
    if (strncasecmp(text.pos, "GET", text.len) == 0)   return true; 
    if (strncasecmp(text.pos, "POST", text.len) == 0)  return true;
    if (strncasecmp(text.pos, "HEAD", text.len) == 0)  return true; 
    return false; 
}

bool HttpParser::check_uri(Text& text) {
    return true; 
}

bool HttpParser::check_version(Text& text) {
    return strncasecmp(text.pos, "HTTP/1.1", text.len) == 0; 
}

bool HttpParser::check_header(Header& header) {
    if (strncasecmp(header.key, "Content-length", header.k_len) == 0) {
        content_length = stoi(string(header.value, header.v_len)); 
    } else if (strncasecmp(header.key, "Connection", header.k_len) == 0) {
        if (strncasecmp(header.value, "keep-alive", header.v_len) == 0) {
            keep_alive = true;  
        }
    }
    return true;
}


void HttpParser::inspect() {
    if (done && parse_result == ParseResult::SUCCESS) {
        cout << string(method.pos, method.len) << " "
             << string(uri.pos, uri.len) << " "
             << string(version.pos, version.len) << endl; 
        if (headers.size() > 0) {
            for (auto& h : headers) {
                cout << string(h.key, h.k_len) << ": " << string(h.value, h.v_len) << endl; 
            }
        }
        cout << endl;
        if (content.pos != nullptr && content.len != 0) {
            cout << string(content.pos, content.len) << endl; 
        }
    }
}