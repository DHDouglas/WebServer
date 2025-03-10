#pragma once

#include <netinet/in.h>
#include <sys/socket.h>
#include <string>

class InetAddress {
public:
    InetAddress(std::string ip, uint16_t port, bool ipv6 = false); 
    explicit InetAddress(uint16_t port = 0, bool loopbackOnly = false, bool ipv6 = false);
    explicit InetAddress(const struct sockaddr_in& addr) : addr_(addr) {}
    explicit InetAddress(const struct sockaddr_in6& addr) : addr6_(addr) {}

    void setSockAddrInet6(const struct sockaddr_in6& addr) { addr6_ = addr; }

    sa_family_t getFamily() const { return addr_.sin_family; }
    std::string getIpPortString() const;
    std::string getIpString() const;
    uint16_t getPort() const;
    socklen_t getAddrLen() const; 
    uint32_t ipv4NetEndian() const { return addr_.sin_addr.s_addr; }
    uint16_t portNetEndian() const { return addr_.sin_port; }
    const struct sockaddr* getSockAddr() const;

    static struct sockaddr* sockAddrCast(struct sockaddr_in* addr);
    static struct sockaddr* sockAddrCast(struct sockaddr_in6* addr);
    static const struct sockaddr* sockAddrCast(const struct sockaddr_in* addr);
    static const struct sockaddr* sockAddrCast(const struct sockaddr_in6* addr);

    static struct sockaddr_in6 getLocalAddrBySockfd(int sockfd); 
    static struct sockaddr_in6 getPeerAddrBySockfd(int sockfd); 

private:
    union {
        struct sockaddr_in addr_; 
        struct sockaddr_in6 addr6_; 
    }; 
};
