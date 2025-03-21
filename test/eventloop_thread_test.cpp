#include <cstdio>
#include <thread>
#include <unistd.h>
#include <iostream>

#include "eventloop.h"
#include "eventloop_thread.h"

using namespace  std;

/* 测试EvetLoopThread类的封装 */

void runInThread()
{
  cout << "runInThread(): pid = " << getpid() 
       << "tid = " << this_thread::get_id() << endl; 
}

int main() {
    cout << "main(): pid = " << getpid() 
         << "tid = " << this_thread::get_id() << endl; 
    EventLoopThread loopThread;
    EventLoop* loop = loopThread.startLoop();
    loop->runInLoop(runInThread);
    sleep(1);
    loop->runAfter(Timestamp::secondsToDuration(2), runInThread);
    sleep(3);
    loop->quit();

    printf("exit main().\n");
}