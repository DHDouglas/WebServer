#pragma once

#include <atomic>
#include <vector>
#include <cstring>
#include <string>
// #include <>


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
    Buffer(int initBuffSize = kInitialSize); 
    ~Buffer() = default;  

    size_t WritableBytes() const;       // 缓冲区中可写的字节数
    size_t ReadableBytes() const;       // 缓冲区中可读的字节数
    size_t PrependableBytes() const ;   // 缓冲区中, 位于readPos位置之前的字节数. 

    const char* Peek() const;           // 返回readPos位置. 
    char* BeginWrite();                 // 返回writePos位置. 
    const char* BeginWrite() const;     // ? 为什么需要重载一个const版本? 哪里会用到? 


    // 从缓冲区中读/检索, 移动readPos. 
    void Retrieve(size_t len);               // [Peek(), Peek()+len)  
    void RetrieveUntil(const char* end);     // [Peek(), end), 不包括end
    void RetrieveAll(); 
    std::string RetrieveToStr(size_t len);   // [Peek(), Peek()+len), 转为str. 
    std::string RetrieveAllToStr();          

    
    void EnsureWriteable(size_t len);   // 确保可写. 
    void HasWritten(size_t len);        // 记录已写入的字节数, 移动writePos. 

    // 四个Append重载版本. 
    void Append(const std::string& str);
    void Append(const char* str, size_t len);   // 基础版本.
    void Append(const void* data, size_t len);  
    // void Append(const Buffer& buff); // ? 不确定什么情况会用到


    ssize_t ReadFd(int fd, int* Errno);     // 从fd中读取到缓冲区
    // ssize_t WriteFd(int fd, int* Errno);    // 向fd中写入缓冲区内容. 

    static const size_t kInitialSize = 1024; 


private:
    void MakeSpace_(size_t len);     // 缓冲区扩容 or 移动readable数据. 
    char* BeginPtr_();               // 返回buffer起始地址. 
    const char* BeginPtr_() const;   // 返回buffer起始地址. 

    std::vector<char> buffer_;
    std::atomic<std::size_t> readPos_;
    std::atomic<std::size_t> writePos_; 
};