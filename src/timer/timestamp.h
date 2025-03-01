#pragma once

#include <chrono>
#include <string>
#include <sys/time.h>

class Timestamp {
public:
    using Clock = std::chrono::steady_clock;     // 单调时钟
    using Duration = std::chrono::microseconds;  // 微秒数   
    using TimePoint = std::chrono::time_point<Clock, Duration>; // 微秒单位的时间点

    static constexpr int kMicroSecondsPerSecond = 1000 * 1000; 
    
public:
    Timestamp() : time_point_(Duration(0)) {}
    
    explicit Timestamp(int64_t us) : time_point_(Duration(us)) {}

    explicit Timestamp(TimePoint tp) : time_point_(tp) {}

    static Timestamp now() {
        return Timestamp(std::chrono::time_point_cast<Duration>(Clock::now()));  
    }
    
    static Timestamp invalid() {
        return Timestamp(0); 
    }

    Duration time_since_epoch() const {
        // return std::chrono::duration_cast<Duration>(time_point_.time_since_epoch());
        return time_point_.time_since_epoch();
    }

    static Duration secondsToDuration(double s) {
        return Duration(static_cast<int>(s * kMicroSecondsPerSecond)); 
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

    std::string toString() const {
        return std::to_string(time_point_.time_since_epoch().count());
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