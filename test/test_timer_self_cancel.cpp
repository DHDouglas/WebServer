#include <iostream>
#include <memory>
#include <unistd.h>
#include <iostream>

#include "eventloop.h"

using namespace std;
EventLoop* g_loop; 
weak_ptr<Timer> wk_ptr_timer;

void cancelSelf() {
    cout << "cancelSelf()" << endl; 
    g_loop->removeTimer(wk_ptr_timer); 
    g_loop->quit(); 
}

void print(string msg, int& times) {
    cout << "msg " << Timestamp::now().toString() << " " << msg << " x" << ++times << endl;
}

int main() {
    EventLoop loop;
    g_loop = &loop;
    loop.runAfter(Timestamp::secondsToDuration(1), bind(print, "once1", 0)); 
    loop.runAfter(Timestamp::secondsToDuration(1.5), bind(print, "once1.5", 0)); 
    loop.runAfter(Timestamp::secondsToDuration(2.5), bind(print, "once2.5", 0)); 
    loop.runAfter(Timestamp::secondsToDuration(3.5), bind(print, "once3.5", 0)); 
    loop.runEvery(Timestamp::secondsToDuration(2), bind(print, "every2", 0)); 
    loop.runEvery(Timestamp::secondsToDuration(3), bind(print, "every3", 0)); 
    wk_ptr_timer = loop.runEvery(Timestamp::secondsToDuration(5), cancelSelf);
    loop.loop(); 
}