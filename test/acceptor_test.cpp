#include <iostream>
#include <netinet/in.h>
#include <cstring>
#include <unistd.h>

#include "eventloop.h"
#include "acceptor.h"
#include "inet_address.h"

using namespace std;


void newConnection(int sockfd, const InetAddress& addr) {
    cout << "newConnection(): accepted a new connection from " 
         << addr.getPort() << endl; 
    const char* msg = "Hello World!\n";
    write(sockfd, msg, strlen(msg)); 
    close(sockfd); 
}

int main() {
    int port = 8889;
    InetAddress addr(port); 
    EventLoop loop;
    Acceptor acceptor(&loop, addr); 
    acceptor.setNewConnCallback(newConnection);  
    acceptor.listen(); 
    loop.loop(); 
}