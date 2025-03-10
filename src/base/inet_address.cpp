#include "inet_address.h"

#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cstddef>

#include "logger.h"


using namespace std;

static_assert(sizeof(InetAddress) == sizeof(struct sockaddr_in6),
              "InetAddress is same size as sockaddr_in6");
static_assert(offsetof(sockaddr_in, sin_family) == 0, "sin_family offset 0");
static_assert(offsetof(sockaddr_in6, sin6_family) == 0, "sin6_family offset 0");
static_assert(offsetof(sockaddr_in, sin_port) == 2, "sin_port offset 2");
static_assert(offsetof(sockaddr_in6, sin6_port) == 2, "sin6_port offset 2");

InetAddress::InetAddress(uint16_t port, bool loop_back_only, bool ipv6) {
    if (ipv6) {
        memset(&addr6_, 0, sizeof(addr6_)); 
        addr6_.sin6_family = AF_INET6; 
        in6_addr ip = loop_back_only ? in6addr_loopback : in6addr_any; 
        addr6_.sin6_addr = ip;
        addr6_.sin6_port = htons(port); 
    } else {
        memset(&addr_, 0, sizeof(addr_));
        addr_.sin_family = AF_INET; 
        in_addr_t ip = loop_back_only ? INADDR_LOOPBACK : INADDR_ANY; 
        addr_.sin_addr.s_addr = htonl(ip); 
        addr_.sin_port = htons(port); 
    }
}

InetAddress::InetAddress(string ip, uint16_t port, bool ipv6) {
    if (ipv6 || strchr(ip.c_str(), ':')) {
        memset(&addr6_, 0, sizeof(addr6_)); 
        addr6_.sin6_family = AF_INET6;
        addr6_.sin6_port = htons(port); 
        if (::inet_pton(AF_INET6, ip.c_str(), &addr6_.sin6_addr) <= 0) {
            LOG_SYSERR << "InetAddress::InetAddress inet_pton"; 
        }
    } else {
        memset(&addr_, 0, sizeof(addr_)); 
        addr_.sin_family = AF_INET;
        addr_.sin_port = htons(port); 
        if (::inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr) <= 0) {
            LOG_SYSERR << "InetAddress::InetAddress inet_pton"; 
        }
    }
}

string InetAddress::getIpPortString() const {
    return getIpString() + ":" + to_string(getPort()); 
}

string InetAddress::getIpString() const {
    char buf[64] = ""; 
    if (addr_.sin_family == AF_INET) {
        ::inet_ntop(AF_INET, &addr_.sin_addr, buf, static_cast<socklen_t>(sizeof(buf)));
    } else {
        ::inet_ntop(AF_INET6, &addr6_.sin6_addr, buf, static_cast<socklen_t>(sizeof(buf)));
    }
    return buf; 
}

uint16_t InetAddress::getPort() const {
    return ntohs(addr_.sin_port);  // sockaddr_in与sockaddr_in6中sin_port偏移相同. 
}

const struct sockaddr* InetAddress::getSockAddr() const {
    return sockAddrCast(&addr_);; 
}

socklen_t InetAddress::getAddrLen() const {
    if (addr_.sin_family == AF_INET) {
        return sizeof(addr_); 
    } else {
        return sizeof(addr6_);
    }
}


struct sockaddr* InetAddress::sockAddrCast(struct sockaddr_in* addr) {
    return static_cast<struct sockaddr*>(static_cast<void*>(addr));
}

struct sockaddr* InetAddress::sockAddrCast(struct sockaddr_in6* addr) {
    return static_cast<struct sockaddr*>(static_cast<void*>(addr));
}

const struct sockaddr* InetAddress::sockAddrCast(const struct sockaddr_in* addr) {
    return static_cast<const struct sockaddr*>(static_cast<const void*>(addr));
}

const struct sockaddr* InetAddress::sockAddrCast(const struct sockaddr_in6* addr) {
    return static_cast<const struct sockaddr*>(static_cast<const void*>(addr));
}


struct sockaddr_in6 InetAddress::getLocalAddrBySockfd(int sockfd) {
    struct sockaddr_in6 addr6_;
    memset(&addr6_, 0, sizeof(addr6_)); 
    socklen_t addrlen = static_cast<socklen_t>(sizeof(addr6_));
    if (::getsockname(sockfd, InetAddress::sockAddrCast(&addr6_), &addrlen) < 0) {
        LOG_SYSERR << "InetAddress::getLocalAddrBySockfd";
    }
    return addr6_;
}

struct sockaddr_in6 InetAddress::getPeerAddrBySockfd(int sockfd) {
    struct sockaddr_in6 addr6_;
    memset(&addr6_, 0, sizeof(addr6_)); 
    socklen_t addrlen = static_cast<socklen_t>(sizeof(addr6_));
    if (::getpeername(sockfd, InetAddress::sockAddrCast(&addr6_), &addrlen) < 0) {
      LOG_SYSERR << "InetAddress::getPeerAddrBySockfd";
    }
    return addr6_;
}