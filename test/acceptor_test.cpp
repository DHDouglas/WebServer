#include <iostream>
#include <netinet/in.h>

#include "eventloop.h"
#include "acceptor.h"

using namespace std;


void newConnection(int sockfd, struct sockaddr_in addr) {
    cout << "newConnection(): accepted a new connection from " 
         << ntohs(addr.sin_port) << endl; 
    const char* msg = "Hello World!\n";
    write(sockfd, msg, strlen(msg)); 
    close(sockfd); 
}


int main() {
    int port = 8889;
    EventLoop loop;
    Acceptor acceptor(&loop, port); 
    acceptor.setNewConnCallBack(newConnection);  
    acceptor.listen(); 
    loop.loop(); 
}