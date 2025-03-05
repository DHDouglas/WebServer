#include <unistd.h>
#include <iostream>
// #include <thread>

#include "thread.h"

using namespace std;

void threadFunc(int id) {
    cout << "Thread(" << id << "): "<< gettid() << endl;
}

int main() {
    cout << "Main(p): " << getpid() << endl; 
    cout << "Main(t): " << gettid() << endl; 
    // std::thread t1(threadFunc, 1);
    // std::thread t2(threadFunc, 2);
    // t1.join();
    // t2.join();

    Thread t1(bind(threadFunc, 1));
    Thread t2(bind(threadFunc, 2));
    t1.start();
    t2.start();
    t1.join();
    t2.join();
}