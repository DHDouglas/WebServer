#include "logger.h"

Logger::LogLevel g_log_level = Logger::INFO; 

// 默认日志输出回调: 输出到stdout
void defaultOutput(const char* msg, int len) {
    fwrite(msg, 1, len, stdout); 
}

void defaultFlush() {
    fflush(stdout); 
}

Logger::OutputFunc g_output = defaultOutput;
Logger::FlushFunc g_flush = defaultFlush;


const char* LogLevelName[Logger::NUM_LOG_LEVELS] = {
  "TRACE ",
  "DEBUG ",
  "INFO  ",
  "WARN  ",
  "ERROR ",
  "FATAL ",
};


// 缓存上次计算的日期时间字符串(精度到秒)
thread_local time_t t_last_second; 
thread_local char t_time[64];
// 缓存errno错误信息
thread_local char t_errnobuf[512];

const char* strerror_tl(int savedErrno) {
    return strerror_r(savedErrno, t_errnobuf, sizeof t_errnobuf);
}

Logger::Logger(SourceFile file, int line)
    : impl_(INFO, 0, file, line) { }

Logger::Logger(SourceFile file, int line, LogLevel level)
    : impl_(level, 0, file, line) { }

Logger::Logger(SourceFile file, int line, LogLevel level, const char* func)
    : impl_(level, 0, file, line)
{
    impl_.stream_ << func << ' '; 
}

Logger::Logger(SourceFile file, int line, bool to_abort)
    : impl_(to_abort?FATAL:ERROR, errno, file, line) { }

Logger::Impl::Impl(LogLevel level, int savedErrno, const SourceFile& file, int line)
    : time_(Timestamp::now()), 
    stream_(),
    level_(level),
    line_(line),
    basename_(file)
{
    formatTime(); 
    CurrentThread::getTid(); 
    stream_.append(CurrentThread::tidString(), CurrentThread::tidStringLength()); 
    stream_.append(LogLevelName[level], 6); 
    if (savedErrno != 0) {
        stream_ << strerror_tl(savedErrno) << " (errno=" << savedErrno << ") ";
    }
}

void Logger::Impl::formatTime() {
    int64_t us_since_epoch = time_.getMicroSecondsSinceEpoch(); 
    time_t seconds = static_cast<time_t>(us_since_epoch / Timestamp::kMicroSecondsPerSecond); 
    int remainder_us = static_cast<int>(us_since_epoch % Timestamp::kMicroSecondsPerSecond); 

    if (seconds != t_last_second) {  
        t_last_second = seconds; 
        struct tm tm;
        localtime_r(&seconds, &tm);   // 转本地时间
        // 缓存
        int len = snprintf(t_time, sizeof(t_time), "%4d%02d%02d %02d:%02d:%02d", 
            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, 
            tm.tm_hour, tm.tm_min, tm.tm_sec); 
        assert(len == 17); 
    }

    Fmt us(".%06d ", remainder_us);  
    assert(us.length() == 8);
    stream_.append(t_time, 17);
    stream_.append(us.data(), 8); 
}


inline LogStream& operator<<(LogStream& s, const Logger::SourceFile& v) {
    s.append(v.data_, v.size_); 
    return s; 
}

void Logger::Impl::finish() {
    stream_ << " - " << basename_ << ':' << line_ << '\n'; 
}

// 析构时调用回调函数
Logger::~Logger() {
    impl_.finish(); 
    // 将LogStream中的buffer传给output回调, 由后者确定具体写入到哪
    const auto& buf(stream().buffer()); 
    g_output(buf.data(), buf.length());  
    if (impl_.level_ == FATAL) {
        g_flush(); 
        abort(); 
    }
}

void Logger::setLogLevel(Logger::LogLevel level) {
  g_log_level = level;
}

void Logger::setOutput(OutputFunc out) {
  g_output = out;
}

void Logger::setFlush(FlushFunc flush) {
  g_flush = flush;
}