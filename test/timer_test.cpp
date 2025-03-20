#include <iostream>
#include <unistd.h>
#include <thread>
#include <iostream>

#include "channel.h"
#include "eventloop.h"

using namespace std;

int cnt = 0;
EventLoop* g_loop; 

void print_tid() {
    cout << "pid = " << getpid() << ", tid = " << this_thread::get_id(); 
    cout << "now: " << Timestamp::now().toFormattedString() << endl; 
}

void print(string msg, int& times) {
    cout << "msg " << Timestamp::now().toFormattedString() << " " << msg << " x" << ++times << endl;
    if (++cnt == 20) {
        g_loop->quit(); 
    }
}

int main() {
    print_tid();
    EventLoop loop;
    g_loop = &loop;

    cout << "main" << endl; 
    loop.runAfter(Timestamp::secondsToDuration(1), bind(print, "once1", 0)); 
    loop.runAfter(Timestamp::secondsToDuration(1.5), bind(print, "once1.5", 0)); 
    loop.runAfter(Timestamp::secondsToDuration(2.5), bind(print, "once2.5", 0)); 
    loop.runAfter(Timestamp::secondsToDuration(3.5), bind(print, "once3.5", 0)); 
    loop.runEvery(Timestamp::secondsToDuration(2), bind(print, "every2", 0)); 
    loop.runEvery(Timestamp::secondsToDuration(3), bind(print, "every3", 0)); 
    loop.loop(); 
    cout << "main loop exits" << endl; 
    sleep(1);
}