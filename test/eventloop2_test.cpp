#include "eventloop.h"

#include <thread.h>

EventLoop* g_loop; 

/* 测试EventLoop.loop()只能在其所属线程被调用 */

void thread_func() {
    g_loop->loop();
}

int main() {
    EventLoop mainLoop;
    g_loop = &mainLoop; 
    std::thread t(thread_func); 
    t.join();
}