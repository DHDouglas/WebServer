#pragma once

#include <chrono>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <string>
#include <sys/time.h>

class Timestamp {
public:
    using Clock = std::chrono::system_clock;     // 系统时钟
    using Duration = std::chrono::microseconds;  // 微秒数   
    using TimePoint = std::chrono::time_point<Clock, Duration>; // 微秒单位的时间点

    static constexpr int kMicroSecondsPerSecond = 1000 * 1000; 
    
public:
    Timestamp() : time_point_(TimePoint{}) {}

    explicit Timestamp(TimePoint tp) : time_point_(tp) {}

    static Timestamp now() {
        return Timestamp(std::chrono::time_point_cast<Duration>(Clock::now()));  
    }
    
    static Timestamp invalid() {
        return Timestamp(); 
    }

    static Duration secondsToDuration(double s) {
        return Duration(std::llround(s * kMicroSecondsPerSecond)); 
    }

    time_t getMicroSecondsSinceEpoch() const {
        return time_point_.time_since_epoch().count(); 
    }
    
    Duration getDurationSinceEpoch() const {
        // return std::chrono::duration_cast<Duration>(time_point_.time_since_epoch());
        return time_point_.time_since_epoch();
    }

    // 计算该Timestamp指定时间相较于now()的差值, 以timespec类型返回.
    struct timespec getDurationFromNowAsTimeSpec() const {
        int64_t us = (*this - Timestamp::now()).count(); 
        if (us < 100) {  // 保证非负
            us = 100; 
        }
        struct timespec ts;
        ts.tv_sec = static_cast<time_t>(us / kMicroSecondsPerSecond);
        ts.tv_nsec = static_cast<long>((us % kMicroSecondsPerSecond) * 1000);
        return ts;
    }

    std::string toFormattedString(bool show_microseconds = true, bool use_utc = false) const {
        char buf[80] = {0};
        std::time_t tt = Clock::to_time_t(time_point_); 
        struct tm tm;
        if (use_utc) {
            gmtime_r(&tt, &tm);     // UTC时间
        } else {
            localtime_r(&tt, &tm);  // 本地时间
        }
        if (show_microseconds) {
            int remain_us = static_cast<int>(time_point_.time_since_epoch().count() % kMicroSecondsPerSecond); 
            snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d",
            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec, remain_us);
        } else {
            snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d", 
            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, 
            tm.tm_hour, tm.tm_min, tm.tm_sec); 
        }
        return buf;
    }

    // 时间戳相减得到时间间隔
    Duration operator-(const Timestamp& rhs) const {
        return time_point_ - rhs.time_point_;
    }

    // 加减时间间隔得到时间戳
    Timestamp operator+(const Duration& duration) const {
        return Timestamp(time_point_ + duration);
    }

    Timestamp operator-(const Duration& duration) const {
        return Timestamp(time_point_ - duration);
    }

    bool operator<(const Timestamp& other) const { 
        return time_point_ < other.time_point_; 
    }

    bool operator>(const Timestamp& other) const { 
        return time_point_ > other.time_point_; 
    }

    bool operator==(const Timestamp& other) const { 
        return time_point_ == other.time_point_; 
    }

private:
    TimePoint time_point_; 
};