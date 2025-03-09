#include "http_server.h" 

using namespace std;

int main() {
    // 创建 AsyncLogger类, 设置全局日志级别, 日志文件名, 日志参数, 全局日志回调函数, 运行
    string root_path = "/home/yht/Projects/MyTinyWebServer/resources";
    string ip = "";
    int port = 8899; 
    int num_thread = 5; 
    HttpServer server(ip, port, num_thread, root_path, 20);
    server.start();
}