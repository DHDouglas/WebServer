#pragma once

#include <cstddef>
#include <string>
#include <unistd.h>
#include <memory>
#include <mutex>
#include <cassert>
#include <cstdio>
#include <ctime>
#include <sys/types.h>

// 封装对文件的append写入操作
class AppendFile {
public:
    explicit AppendFile(std::string filename);
    ~AppendFile(); 
    void append(const char* logline, size_t len);  // 追加到文件
    void flush();  
    off_t writtenBytes() const; 

private:
    size_t write(const char* logline, size_t len); 

private:
    FILE* fp_; 
    char buffer_[64*1024]; 
    off_t written_bytes_;    // 记录当前文件总写入字节数. 超过roll_size_时换新文件. 
};


// 日志文件类
class LogFile {
public:
    LogFile(const std::string& basename, 
            off_t roll_size, 
            bool thread_safe = true, 
            int fluash_interval = 3, 
            int check_every_n = 1024); 
    ~LogFile(); 

    void append(const char* logline, int len); 
    void flush();
    bool rollFile();   // 创建新一份日志文件

private:
    void appendUnlocked(const char* logline, int len); 
    static std::string getLogFileName(const std::string& basename, time_t* now);  

private:
    const std::string basename_;
    const off_t roll_size_;
    const int flush_interval_;   
    const int check_every_n_;   // n次写入后检查时间, 是否需换新日志, or是否刷入磁盘.

    int count_;

    std::unique_ptr<std::mutex> mtx_;   // 以指针形式, 是因为不一定需要
    std::unique_ptr<AppendFile> file_; 

    time_t start_of_period_;
    time_t last_roll_;
    time_t last_flush_; 

    const static int kRollIntervalSeconds_ = 60 * 60 * 24;  // 日志文件滚动间隔秒数. 默认24h.
};