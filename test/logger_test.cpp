#include "logger.h"
#include <thread>
#include <vector>

using namespace std;

void threadFunc(int id) {
    LOG_TRACE << "[" << id << "]: This is a trace log";
    LOG_DEBUG << "[" << id << "]: This is a debug log";
    LOG_INFO << "[" << id << "]: This is a info log";
    LOG_WARN << "[" << id << "]: This is a warn log";
    LOG_ERROR << "[" << id << "]: This is a error log";
    LOG_SYSERR << "[" << id << "]: This is a syserror log";
}


int main() {
    vector<thread> threads; 
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back(threadFunc, i + 1);
    }
    for (auto& t : threads) {
        t.join(); 
    }

    LOG_INFO << "**************************************";

    threads.clear(); 
    for (int i = 0; i < 6; ++i) {
        Logger::LogLevel level;
        switch (i) {
            case 0: level = Logger::TRACE; break; 
            case 1: level = Logger::DEBUG; break; 
            case 2: level = Logger::INFO; break; 
            case 3: level = Logger::WARN; break; 
            case 4: level = Logger::ERROR; break; 
            case 5: level = Logger::FATAL; break; 
        }
        Logger::setLogLevel(level);
        threads.emplace_back(threadFunc, i + 11);
        threads[i].join();
        LOG_ERROR << "**************************************";
    }
    LOG_SYSFATAL << "This is a sysfatal log and will invoke abort()";
}