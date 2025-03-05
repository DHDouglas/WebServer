#include "eventloop.h"
#include <unistd.h>
#include <thread>
#include <iostream>

using namespace std;

void thread_func1() {
    cout << "thread_func(): pid = " << getpid() 
         << ", tid = " << std::this_thread::get_id() << endl; 
    EventLoop loop;
    loop.loop();
}

int main() {
    cout << "main: pid = " << getpid() 
         << ", tid = " << std::this_thread::get_id() << endl; 
    EventLoop mainLoop;
    mainLoop.loop();
    std::thread t(thread_func1); 
    t.join();
}