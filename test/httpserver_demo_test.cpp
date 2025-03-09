#include "http_server.h" 

int main(int argc, char* argv[]) {
    Config config(argc, argv);  
    HttpServer server(config);
    server.start();
}