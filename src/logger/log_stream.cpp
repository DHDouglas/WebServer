#include "log_stream.h"

#include <algorithm>
#include <cassert>
#include <limits>
#include <cstdio>
#include <cstring>

using namespace std;

const char digits[] = "0123456789";
const char digitsHex[] = "0123456789ABCDEF"; 

template<typename T>
size_t convert(char buf[], T value) {
    T i = value;
    char* p = buf;
    do {
        int d = static_cast<int>(i % 10); 
        i /= 10;
        *p++ = digits[d]; 
    } while (i != 0); 
    if (value < 0) {
        *p++ = '-'; 
    }
    *p = '\0'; 
    std::reverse(buf, p);
    return p - buf; 
}

size_t convertHex(char buf[], uintptr_t value)
{
  uintptr_t i = value;
  char* p = buf;
  do {
    int d = static_cast<int>(i % 16);
    i /= 16;
    *p++ = digitsHex[d];
  } while (i != 0);

  *p = '\0';
  std::reverse(buf, p);
  return p - buf;
}



template <typename T>
void LogStream::formatInteger(T v) {
    // buffer不够时, 直接丢弃, 不写入. 
    if (buffer_.avail() >= kMaxNumericSize) {  
        size_t len = convert(buffer_.current(), v); 
        buffer_.add(len); 
    }
}

LogStream& LogStream::operator<<(short v) {
    *this << static_cast<int>(v);
    return *this;
}

LogStream& LogStream::operator<<(unsigned short v) {
    *this << static_cast<unsigned int>(v);
    return *this;
}

LogStream& LogStream::operator<<(int v) {
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(unsigned int v) {
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(long v) {
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(unsigned long v) {
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(long long v) {
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(unsigned long long v) {
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(float v) {
    *this << static_cast<double>(v);
    return *this; 
}

LogStream& LogStream::operator<<(double v) {
    if (buffer_.avail() >= kMaxNumericSize) {
        int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12g", v); 
        buffer_.add(len); 
    }
    return *this; 
}

LogStream& LogStream::operator<<(char v) {
    buffer_.append(&v, 1);
    return *this;
}

LogStream& LogStream::operator<<(const char* str) {
    if (str) {
        buffer_.append(str, strlen(str)); 
    } else {
        buffer_.append("(null)", 6); 
    }
    return *this; 
}

LogStream& LogStream::operator<<(const unsigned char* str) {
    return operator<<(reinterpret_cast<const char*>(str));
}


LogStream& LogStream::operator<<(const string& str) {
    return operator<<(str.c_str());
}

// 打印指针地址
LogStream& LogStream::operator<<(const void* p) {
    uintptr_t v = reinterpret_cast<uintptr_t>(p);
    if (buffer_.avail() >= kMaxNumericSize) {
        char* buf = buffer_.current();
        buf[0] = '0'; 
        buf[1] = 'x';
        size_t len = convertHex(buf + 2, v); 
        buffer_.add(len + 2); 
    } 
    return *this;
}


void LogStream::staticCheck() {
  static_assert(kMaxNumericSize - 10 > numeric_limits<double>::digits10,
                "kMaxNumericSize is large enough");
  static_assert(kMaxNumericSize - 10 > numeric_limits<long double>::digits10,
                "kMaxNumericSize is large enough");
  static_assert(kMaxNumericSize - 10 > numeric_limits<long>::digits10,
                "kMaxNumericSize is large enough");
  static_assert(kMaxNumericSize - 10 > numeric_limits<long long>::digits10,
                "kMaxNumericSize is large enough");
}



template<typename T>
Fmt::Fmt(const char* fmt, T val) {
    static_assert(std::is_arithmetic<T>::value == true, "Must be arithmetic type");
    length_ = snprintf(buf_, sizeof(buf_), fmt, val);
    assert(static_cast<size_t>(length_) < sizeof(buf_)); 
}

// Explicit instantiations
template Fmt::Fmt(const char* fmt, char);

template Fmt::Fmt(const char* fmt, short);
template Fmt::Fmt(const char* fmt, unsigned short);
template Fmt::Fmt(const char* fmt, int);
template Fmt::Fmt(const char* fmt, unsigned int);
template Fmt::Fmt(const char* fmt, long);
template Fmt::Fmt(const char* fmt, unsigned long);
template Fmt::Fmt(const char* fmt, long long);
template Fmt::Fmt(const char* fmt, unsigned long long);

template Fmt::Fmt(const char* fmt, float);
template Fmt::Fmt(const char* fmt, double);
