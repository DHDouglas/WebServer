
#include "utils.h"
#include <cstdlib>
#include <cstdio>
#include <fcntl.h>
#include <cassert>



int Utils::setnonblockling(int fd) {
    assert(fd > 0); 
    int flags;
    if ((flags = fcntl(fd, F_GETFL)) < 0) {
        perror("F_GETFL");
        exit(EXIT_FAILURE); 
    }
    flags |= O_NONBLOCK;  // 设置文件描述符非阻塞
    if (fcntl(fd, F_SETFL, flags) < 0) {
        perror("F_SETFL"); 
        exit(EXIT_FAILURE); 
    }
    return flags;  // 返回旧标志位. 
}