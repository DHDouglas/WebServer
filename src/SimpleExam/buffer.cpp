#include "buffer.h"
#include <cstddef>
#include <cassert>
#include <sys/uio.h>

using namespace std; 


const size_t Buffer::kInitialSize; 


Buffer::Buffer(int initBuffsize) 
: buffer_(initBuffsize), readPos_(0), writePos_(0) {}

size_t Buffer::ReadableBytes() const {
    return writePos_ - readPos_; 
}

size_t Buffer::WritableBytes() const {
    return buffer_.size() - writePos_; 
}

size_t Buffer::PrependableBytes() const {
    return readPos_;
}

const char* Buffer::Peek() const {
    return BeginPtr_() + readPos_; 
}

void Buffer::Retrieve(size_t len) {
    assert(len <= ReadableBytes()); 
    if (len < ReadableBytes()) {
        readPos_ += len;
    } else {
        RetrieveAll();    // 全部读完了, 重置readPos==writePos为起始位置. 
    }
}

void Buffer::RetrieveUntil(const char* end) {   
    assert(Peek() <= end);
    assert(end <= BeginWrite()); 
    Retrieve(end - Peek());  
}

void Buffer::RetrieveAll() {
    // bzero(&buffer_[0], buffer_.size());  // ? 为什么需要清零整个buffer, 多一笔这开销? 
    readPos_ = 0;
    writePos_ = 0;
}

std::string Buffer::RetrieveAllToStr() {
    return string(RetrieveToStr(ReadableBytes())); 
}

std::string Buffer::RetrieveToStr(size_t len) {
    assert(len <= ReadableBytes()); 
    string str(Peek(), len);
    Retrieve(len); 
    return str;
}


char* Buffer::BeginWrite() {
    return BeginPtr_() + writePos_; 
}

const char* Buffer::BeginWrite() const {
    return BeginPtr_() + writePos_; 
}

void Buffer::HasWritten(size_t len) {
    writePos_ += len; 
}

void Buffer::Append(const char* str, size_t len) {
    assert(str); 
    EnsureWriteable(len); 
    std::copy(str, str + len, BeginWrite()); 
    HasWritten(len); 
}

void Buffer::Append(const std::string& str) {
    Append(str.data(), str.length());  
}

void Buffer::Append(const void* data, size_t len) {
    assert(data); 
    Append(static_cast<const char*>(data), len); 
}


// void Buffer::Append(const Buffer& buff) {

// }


void Buffer::EnsureWriteable(size_t len) {
    if (WritableBytes() < len) {
        MakeSpace_(len); 
    } 
    assert(WritableBytes() >= len); 
}


ssize_t Buffer::ReadFd(int fd, int* saveErrno) {
    // scatter read. 临时利用栈空间作为一部分输入缓冲区, 使得输入缓冲区足够大, 减少系统调用. 
    char extrabuf[65536];

    struct iovec iov[2]; 
    const size_t writable = WritableBytes(); 
    iov[0].iov_base = BeginWrite();
    iov[0].iov_len = writable;  
    iov[1].iov_base = extrabuf;
    iov[1].iov_len = sizeof(extrabuf);  

    // ! 这里只调用了一次read而没有反复read直至返回EAGAIN, 要求是level trigger, 否则可能丢失数据. 
    // level trigger下, 没读完时会再次触发EPOLLIN事件.  

    const ssize_t n = readv(fd, iov, 2);  
    if (n < 0) {
        *saveErrno = errno;
    } else if (static_cast<size_t>(n) <= writable) {
        HasWritten(n);  
    } else {
        HasWritten(writable); 
        Append(extrabuf, n - writable);  // 将extrabuf中的剩余数据追加到buffer_中. 
    }
    return n; 
}



// ssize_t Buffer::WriteFd(int fd, int* saveErrno) {

// }


char* Buffer::BeginPtr_() {
    // return &(*buffer_.begin()); 
    return buffer_.data(); 
}

const char* Buffer::BeginPtr_() const {
    return buffer_.data(); 
}

void Buffer::MakeSpace_(size_t len) {
    if (WritableBytes() + PrependableBytes() < len) {
        buffer_.resize(writePos_ + len);   // 扩容. 
    } else {
        // move readable data to the front, make space inside buffer. 
        size_t readable = ReadableBytes(); 
        std::copy(Peek(), const_cast<const char*>(BeginWrite()), BeginPtr_()); 
        readPos_ = 0;
        writePos_ = readPos_ + readable;
        assert(readable == ReadableBytes()); 
    }
}




