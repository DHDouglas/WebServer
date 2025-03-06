#include "buffer.h"


using namespace std; 


const size_t Buffer::kInitialSize; 
const size_t Buffer::kCheapPrepent; 

Buffer::Buffer(size_t init_size) 
    : buffer_(init_size + kCheapPrepent), 
    read_pos_(kCheapPrepent), 
    write_pos_(kCheapPrepent) 
{
    assert(readableBytes() == 0);
    assert(writableBytes() == init_size);
    assert(prependableBytes() == kCheapPrepent); 
}


void Buffer::swap(Buffer& rhs) {
    buffer_.swap(rhs.buffer_); 
    std::swap(read_pos_, rhs.read_pos_); 
    std::swap(write_pos_, rhs.write_pos_); 
}

size_t Buffer::readableBytes() const {
    return write_pos_ - read_pos_; 
}

size_t Buffer::writableBytes() const {
    return buffer_.size() - write_pos_; 
}

size_t Buffer::prependableBytes() const {
    return read_pos_;
}

char* Buffer::begin() {
    return buffer_.data(); 
}

const char* Buffer::begin() const {
    return buffer_.data(); 
}

const char* Buffer::peek() const {
    return begin() + read_pos_; 
}

char* Buffer::beginWrite() {
    return begin() + write_pos_; 
}

const char* Buffer::beginWrite() const {
    return begin() + write_pos_; 
}

// retrieve定义返回值为void, 避免出现`string str(retrieve(readableBytes(), readableBytes()))`这样的调用
// 由于求值顺序不确定, 所以上述调用是未定义行为. 
// 强制必须分两步, 先获取readableBytes(), 再调用retrieve(). 
void Buffer::retrieve(size_t len) {
    assert(len <= readableBytes()); 
    if (len < readableBytes()) {
        read_pos_ += len;
    } else { 
        retrieveAll();    // 全读完了, 重置readPos==writePos为起始位置. 
    }
}

void Buffer::retrieveUntil(const char* end) {   
    assert(peek() <= end);
    assert(end <= beginWrite()); 
    retrieve(end - peek());  
}

void Buffer::retrieveAll() {  // 重置readPos==writePos为起始位置. 
    read_pos_ = kCheapPrepent;
    write_pos_ = kCheapPrepent;
}

std::string Buffer::retrieveAllToString() {
    return retrieveToString(readableBytes()); 
}

std::string Buffer::retrieveToString(size_t len) {
    assert(len <= readableBytes()); 
    string str(peek(), len);
    retrieve(len); 
    return str;
}

void Buffer::append(const char* str, size_t len) {
    assert(str); 
    ensureWriteable(len); 
    std::copy(str, str + len, beginWrite()); 
    hasWritten(len); 
}

void Buffer::append(const std::string& str) {
    append(str.data(), str.length());  
}

void Buffer::append(const void* data, size_t len) {
    assert(data); 
    append(static_cast<const char*>(data), len); 
}

void Buffer::hasWritten(size_t len) {
    assert(len <= writableBytes());
    write_pos_ += len; 
}

void Buffer::ensureWriteable(size_t len) {
    if (writableBytes() < len) {
        makeSpace(len); 
    } 
    assert(writableBytes() >= len); 
}

void Buffer::makeSpace(size_t len) {
    if (writableBytes() + prependableBytes() < len + kCheapPrepent) {
        buffer_.resize(write_pos_ + len);   // 扩容. 
    } else {
        // move readable data to the front, make space inside buffer. 
        assert(kCheapPrepent < read_pos_); 
        size_t readable = readableBytes(); 
        std::copy(peek(), const_cast<const char*>(beginWrite()), begin() + kCheapPrepent); 
        read_pos_ = kCheapPrepent;
        write_pos_ = read_pos_ + readable;
        assert(readable == readableBytes()); 
    }
}

ssize_t Buffer::readFd(int fd, int* save_errno) {
    // scatter read. 临时利用栈空间作为一部分输入缓冲区, 使得输入缓冲区足够大, 减少系统调用. 
    char extrabuf[65536];
    struct iovec iov[2]; 
    const size_t writable = writableBytes(); 
    iov[0].iov_base = beginWrite();
    iov[0].iov_len = writable;  
    iov[1].iov_base = extrabuf;
    iov[1].iov_len = sizeof(extrabuf);  

    // 只调用了一次read而没有反复read直至返回EAGAIN, 要求是level trigger, 否则可能丢失数据. 
    // 如果buffer_中可写入空间>=extrabuf, 意味着已有64KB. 写一个就足够. 
    const int iovcnt = writable < sizeof(extrabuf) ? 2 : 1; 
    const ssize_t n = readv(fd, iov, iovcnt);  
    if (n < 0) {
        *save_errno = errno;
    } else if (static_cast<size_t>(n) <= writable) {
        hasWritten(n);  
    } else {
        hasWritten(writable); 
        append(extrabuf, n - writable);  // 将extrabuf中的剩余数据追加到buffer_中. 
    }
    return n; 
}





