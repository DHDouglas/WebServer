#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <sys/types.h>

#define PORT 8889
#define BUFFER_SIZE 10 * 1024 * 1024  // 10MB 缓冲区，确保足够大

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // 创建 TCP 套接字
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return 1;
    }

    // 绑定地址和端口
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    // 监听连接
    if (listen(server_fd, 1) == -1) {
        perror("listen");
        close(server_fd);
        return 1;
    }
    std::cout << "服务器正在监听端口 " << PORT << "...\n";

    // 接受客户端连接
    if ((client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len)) == -1) {
        perror("accept");
        close(server_fd);
        return 1;
    }
    std::cout << "客户端已连接\n";

    // 获取 TCP 接收缓冲区大小
    int recv_buf_size;
    socklen_t optlen = sizeof(recv_buf_size);
    getsockopt(client_fd, SOL_SOCKET, SO_RCVBUF, &recv_buf_size, &optlen);
    std::cout << "TCP SO_RCVBUF(实际缓冲区大小): " << recv_buf_size << " 字节\n";

    // 读取数据
    char* buffer = new char[BUFFER_SIZE];
    int max_read_size = 0;

    while (true) {
        int bytes_read = read(client_fd, buffer, BUFFER_SIZE);
        if (bytes_read == 0) {
            std::cout << "客户端已关闭连接\n";
            break;
        }
        if (bytes_read == -1) {
            perror("read");
            break;
        }

        // 记录最大读取值
        if (bytes_read > max_read_size) {
            max_read_size = bytes_read;
        }

        std::cout << "本次 read() 读取 " << bytes_read << " 字节\n";
    }

    std::cout << "最大单次 read() 读取 " << max_read_size << " 字节\n";

    // 释放资源
    delete [] buffer; 
    close(client_fd);
    close(server_fd);

    return 0;
}
