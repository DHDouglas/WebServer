#pragma once 

#include <cstdio>
#include <ctime>

#include "log_stream.h"
#include "timestamp.h"
#include "thread.h"

class AsyncLogger; 

class Logger {
public:
    enum LogLevel {
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL,
        NUM_LOG_LEVELS, 
    };

    // 该类用以去掉文件名的路径前缀. 
    struct SourceFile {
        // 模版参数推导, 编译时确定N, 即字符串长度, 省略运行时调用strlen.
        template <int N>
        SourceFile(const char(&arr)[N]): data_(arr), size_(N-1){
            const char* slash = strrchr(data_, '/'); 
            if (slash) {
                data_ = slash + 1;
                size_ -= static_cast<int>(data_ - arr); 
            }
        }
        // 运行时计算strlen的版本.
        explicit SourceFile(const char* filename): data_(filename) {
            const char* slash = strrchr(filename, '/');
            if (slash) {
                data_ = slash + 1; 
            }
            size_ = static_cast<int>(strlen(data_)); 
        }
        const char* data_;
        int size_; 
    };

public:
    Logger(SourceFile file, int line);
    Logger(SourceFile file, int line, LogLevel level);
    Logger(SourceFile file, int line, LogLevel level, const char* func);
    Logger(SourceFile file, int line, bool to_abort); 
    ~Logger(); 

    LogStream& stream();

    static LogLevel getLogLevel(); 
    static void setLogLevel(LogLevel level); 

    using OutputFunc = void(*)(const char* msg, int len);
    using FlushFunc = void(*)(); 

    static void setOutput(OutputFunc);  // Logger输出回调, 指定具体输出操作, 写到哪.
    static void setFlush(FlushFunc); 

private:
    struct Impl {
        using LogLevel = Logger::LogLevel;
        Impl(LogLevel level, int old_errno, const SourceFile& file, int line); 
        void formatTime();   // 格式化时间并写入日志流
        void finish();  
        
        Timestamp time_;        // 日志时间戳
        LogStream stream_;      // 日志流, 提供`<<`重载以接收日志内容(内有小缓冲区)
        LogLevel level_;        // 日志
        int line_;              // 代码行号
        SourceFile basename_;   // 调用文件名basename
    };

    Impl impl_; 
};

// 日志级别、输出回调、flush回调.
extern Logger::LogLevel g_log_level; 
extern Logger::OutputFunc g_output; 
extern Logger::FlushFunc g_flush;  

inline Logger::LogLevel Logger::getLogLevel() {
    return g_log_level; 
}

inline LogStream& Logger::stream() {
    return impl_.stream_; 
}

// 定义日志宏
#define LOG_TRACE if (Logger::getLogLevel() <= Logger::TRACE) \
    Logger(__FILE__, __LINE__, Logger::TRACE, __func__).stream()

#define LOG_DEBUG if (Logger::getLogLevel() <= Logger::DEBUG) \
    Logger(__FILE__, __LINE__, Logger::DEBUG, __func__).stream()

#define LOG_INFO if (Logger::getLogLevel() <= Logger::INFO) \
    Logger(__FILE__, __LINE__).stream()

#define LOG_WARN     Logger(__FILE__, __LINE__, Logger::WARN).stream()
#define LOG_ERROR    Logger(__FILE__, __LINE__, Logger::ERROR).stream()
#define LOG_FATAL    Logger(__FILE__, __LINE__, Logger::FATAL).stream()
#define LOG_SYSERR   Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL Logger(__FILE__, __LINE__, true).stream()

const char* strerror_tl(int savedErrno);
