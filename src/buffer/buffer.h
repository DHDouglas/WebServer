#pragma once

#include <vector>
#include <string>

/* muduo中buffer的简化实现.
 * 
 * 使用`vector<char>`作为缓冲区, 因此支持自动扩容. 由此需要注意:
 * 扩容时涉及到开辟新内存以及数据的拷贝迁移, 所以缓冲区没有固定地址, 
 * 故提供通过函数`BeginPtr_`动态获取起始地址. 
 * 同时, readPos_与writePos_采用"下标"进行记录, 而非指针, 旨在避免内存地址变化后"迭代器失效"的问题. 
 * 
 * 注:
 * BeginStr_()重载了一个const的版本, 是在其他const成员函数中会需要被调用(例如Peek中), 这就要求const重载. 
 * 
 * FAQ:
 * 为什么需要封装Retrieve函数和HasWritten函数, 单独来移动pos, 而不是直接移动? 
 */


/* A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
 * 
 *  @code
 *  +-------------------+------------------+------------------+
 *  | prependable bytes |  readable bytes  |  writable bytes  |
 *  |                   |     (CONTENT)    |                  |
 *  +-------------------+------------------+------------------+
 *  |                   |                  |                  |
 *  0      <=      readerIndex   <=   writerIndex    <=     size
 *  @endcode
 */

class Buffer {
public:
    explicit Buffer(size_t init_size = kInitialSize); 
    ~Buffer() = default;  

    void swap(Buffer& rhs); 

    size_t writableBytes() const;       // 缓冲区中可写的字节数
    size_t readableBytes() const;       // 缓冲区中可读的字节数
    size_t prependableBytes() const ;   // 缓冲区中, 位于readPos位置之前的字节数. 

    const char* peek() const;           // 返回readPos位置. 
    char* beginWrite();                 // 返回writePos位置. 
    const char* beginWrite() const;     

    // 代表已从缓冲区中读走数据, 仅移动readPos. 
    void retrieve(size_t len);               // [Peek(), Peek()+len)  
    void retrieveUntil(const char* end);     // [Peek(), end), 不包括end
    void retrieveAll(); 
    std::string retrieveToString(size_t len);   // [Peek(), Peek()+len), 转为str. 
    std::string retrieveAllToString();          
    
    void ensureWriteable(size_t len);        // 确保可写. 
    void hasWritten(size_t len);             // 记录已写入的字节数, 移动writePos. 

    void append(const char* str, size_t len);   // 基础版本.
    void append(const std::string& str);
    void append(const void* data, size_t len);  

    ssize_t readFd(int fd, int* save_errno);     // 从fd中读取到缓冲区
    // ssize_t WriteFd(int fd, int* save_errno);    // 向fd中写入缓冲区内容. 

public:
    static const size_t kInitialSize = 1024;
    static const size_t kCheapPrepent = 8;  // 初始时缓冲区头部prependable的大小.

private:
    void makeSpace(size_t len);     // 缓冲区扩容 or 移动readable数据. 
    char* begin();                // 返回buffer起始地址. 
    const char* begin() const;    // 返回buffer起始地址. 

    std::vector<char> buffer_;
    std::size_t read_pos_;
    std::size_t write_pos_; 
};