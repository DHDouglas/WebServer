#include "eventloop.h"

EventLoop* g_loop; 

void thread_func() {
    g_loop->loop();
}

int main() {
    EventLoop mainLoop;
    g_loop = &mainLoop; 
    std::thread t(thread_func); 
    t.join();
}