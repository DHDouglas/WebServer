#pragma once

#include <string>
#include <cstring>

constexpr int kLargeBuffer = 4000 * 1000;  // 4MB
constexpr int kSmallBuffer = 4000;         // 4KB

// 固定大小的缓冲区类
template <int SIZE>
class FixedBuffer {  
public:
    FixedBuffer() : cur_(data_) {}
    ~FixedBuffer() {}

    const char* data() const { return data_; }

    void append(const char* buf, size_t len) {
        if (static_cast<size_t>(avail()) > len) {  // buffer不够时直接丢弃了
            memcpy(cur_, buf, len); 
            cur_ += len;
        }
    }

    char* current() { return cur_; }
    int length() const { return static_cast<int>(cur_ - data_); }
    int avail() const { return static_cast<int>(end() - cur_); }
    void add(size_t len) { cur_ += len; }
    void reset() { cur_ = data_; }
    void bzero() { memset(data_, 0, sizeof(data_));} 

private:
    const char* end() const { return data_ + sizeof(data_); }

private:
    char data_[SIZE]; 
    char* cur_; 
};


class LogStream {
public:
    using Buffer = FixedBuffer<kSmallBuffer>; 

    LogStream& operator<<(bool); 
    LogStream& operator<<(short);
    LogStream& operator<<(unsigned short); 
    LogStream& operator<<(int); 
    LogStream& operator<<(unsigned int);
    LogStream& operator<<(long);
    LogStream& operator<<(unsigned long);
    LogStream& operator<<(long long);
    LogStream& operator<<(unsigned long long); 
    LogStream& operator<<(float);
    LogStream& operator<<(double);
    LogStream& operator<<(char); 
    LogStream& operator<<(const char*);
    LogStream& operator<<(const unsigned char*);
    LogStream& operator<<(const std::string&);
    LogStream& operator<<(const void*);
    // LogStream& operator<<(const Buffer&); 

    void append(const char* data, int len) { buffer_.append(data, len); }
    const Buffer& buffer() const { return buffer_; }
    void resetBuffer() { buffer_.reset(); }

private:
    void staticCheck(); 

    template <typename T>
    void formatInteger(T); 

private:
    Buffer buffer_; 
    static const int kMaxNumericSize = 48;  // 允许的最大数字长度
};


// 封装snprintf进行格式化
class Fmt {
public:
    template <typename T>
    Fmt(const char* fmt, T val); 

    const char* data() const { return buf_; }
    int length() const { return length_; }

private:
    char buf_[80]; 
    int length_;
};
