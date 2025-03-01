#include <sys/socket.h>   // 提供socket, AF_INET, SOCK_STREAM等
#include <netinet/in.h>   // 提供套接字地址结构体sockaddr_in, sockaddr
#include <stdio.h>         
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

#define PORT 8889
#define IP "127.0.0.1"
#define BUF_SIZE 1024


int main() {
    int client_fd = 0;
    struct sockaddr_in serv_addr;    // 套接字地址结构体

    // 创建socket, 返回一个文件描述符fd
    // 参数一: 协议簇, AF_INET表示使用32位IPv4地址; 
    // 参数二: 套接字类型, SOCK_STREAM表示为"字节流套接字". 
    // 参数三: 协议类型, 可选项为`IPPROTO_TCP`等, 设置为0表示使用参数一与参数二组合的系统默认值. 
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // 指定服务器的socket地址——IP地址:端口号
    // IP地址、端口号都需要从主机的小端序转换为"网络字节序"(大端序).
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    // 将IP地址从点分十进制字符串转换为32位大端序的二进制值
    if (inet_pton(AF_INET, IP, &serv_addr.sin_addr) <= 0) {
        perror("无效的地址或地址不支持\n");
        exit(EXIT_FAILURE);
    }

    // 等待连接至指定服务器. connect函数会阻塞等待连接完成. 
    // sockaddr_in* 指针需要转换为指向 "通用套接字地址结构体" 的指针, 即sockaddr*
    if (connect(client_fd, (struct sockaddr*)&serv_addr , sizeof(serv_addr)) < 0) {
        perror("连接失败\n");
        exit(EXIT_FAILURE);
    }

    printf("成功连接到服务器\n");

    // 发送消息到服务器
    const int MSG_SIZE = 65535 * 10;  
    char msg[MSG_SIZE] = {0}; 
    memset(msg, 'A', sizeof(msg) - 1);  
    msg[MSG_SIZE - 1] = '\0'; 
    send(client_fd, msg, MSG_SIZE, 0); 
    printf("消息已发送\n");

    // 关闭套接字
    close(client_fd);
    return 0;
}