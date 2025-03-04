#include <unistd.h>
#include <cstdio>

#include "eventloop.h"

/* 单线程下测试run_in_loop与queue_in_loop */

EventLoop* g_loop;
int g_flag = 0;

void run4() {
  printf("run4(): pid = %d, flag = %d\n", getpid(), g_flag);
  g_loop->quit();
}

void run3() {
  printf("run3(): pid = %d, flag = %d\n", getpid(), g_flag);
  g_loop->runAfter(Timestamp::secondsToDuration(3), run4);
  g_flag = 3;
}

void run2() {
  printf("run2(): pid = %d, flag = %d\n", getpid(), g_flag);
  g_loop->addToQueueInLoop(run3);  // 将在doPendingFunctor中执行. 
}

void run1() {
    g_flag = 1;
    printf("run1(): pid = %d, flag = %d\n", getpid(), g_flag); 
    g_loop->runInLoop(run2); 
    g_flag = 2;
}

int main() {
    printf("main(): pid = %d, flag = %d\n", getpid(), g_flag);

    EventLoop loop;
    g_loop = &loop;
  
    loop.runAfter(Timestamp::secondsToDuration(2), run1);
    loop.loop();
    printf("main(): pid = %d, flag = %d\n", getpid(), g_flag);
}