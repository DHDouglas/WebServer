#include "gtest/gtest.h"
#include <netinet/in.h>
#include "inet_address.h"


TEST(InetAddressTest, IPv4_Construction) {
    InetAddress addr("192.168.1.1", 8080);

    EXPECT_EQ(addr.getFamily(), AF_INET);
    EXPECT_EQ(addr.getIpString(), "192.168.1.1");
    EXPECT_EQ(addr.getPort(), 8080);

    const struct sockaddr* sa = addr.getSockAddr();
    EXPECT_EQ(sa->sa_family, AF_INET);
}

TEST(InetAddressTest, IPv4_Loopback) {
    InetAddress addr(8080, true, false);  // 仅 loopback

    EXPECT_EQ(addr.getFamily(), AF_INET);
    EXPECT_EQ(addr.getIpString(), "127.0.0.1");
    EXPECT_EQ(addr.getPort(), 8080);

    const struct sockaddr* sa = addr.getSockAddr();
    EXPECT_EQ(sa->sa_family, AF_INET);
}

TEST(InetAddressTest, IPv4_AnyAddress) {
    InetAddress addr(9090, false, false);  // 监听所有地址

    EXPECT_EQ(addr.getFamily(), AF_INET);
    EXPECT_EQ(addr.getIpString(), "0.0.0.0");
    EXPECT_EQ(addr.getPort(), 9090);

    const struct sockaddr* sa = addr.getSockAddr();
    EXPECT_EQ(sa->sa_family, AF_INET);
}

TEST(InetAddressTest, IPv6_Construction) {
    InetAddress addr("::1", 8080, true);

    EXPECT_EQ(addr.getFamily(), AF_INET6);
    EXPECT_EQ(addr.getIpString(), "::1");
    EXPECT_EQ(addr.getPort(), 8080);

    const struct sockaddr* sa = addr.getSockAddr();
    EXPECT_EQ(sa->sa_family, AF_INET6);
}

TEST(InetAddressTest, IPv6_Loopback) {
    InetAddress addr(8080, true, true);  // 仅 loopback

    EXPECT_EQ(addr.getFamily(), AF_INET6);
    EXPECT_EQ(addr.getIpString(), "::1");
    EXPECT_EQ(addr.getPort(), 8080);

    const struct sockaddr* sa = addr.getSockAddr();
    EXPECT_EQ(sa->sa_family, AF_INET6);
}

TEST(InetAddressTest, IPv6_AnyAddress) {
    InetAddress addr(9090, false, true);  // 监听所有地址

    EXPECT_EQ(addr.getFamily(), AF_INET6);
    EXPECT_EQ(addr.getIpString(), "::");
    EXPECT_EQ(addr.getPort(), 9090);

    const struct sockaddr* sa = addr.getSockAddr();
    EXPECT_EQ(sa->sa_family, AF_INET6);
}

// IPv6 地址解析失败的情况
TEST(InetAddressTest, InvalidAddressHandling) {
    InetAddress addr("invalid_ip", 8080, false);  // 传入无效 IP

    // 这里 getIpString() 可能返回空，或者保持默认值
    EXPECT_TRUE(addr.getIpString().empty() || addr.getIpString() == "0.0.0.0");
    EXPECT_EQ(addr.getPort(), 8080);
}

// 确保 sockaddr 指针正确指向
TEST(InetAddressTest, SockAddrPointerCheck) {
    InetAddress addr4("192.168.1.1", 8080);
    InetAddress addr6("::1", 8080, true);

    const struct sockaddr* sa4 = addr4.getSockAddr();
    const struct sockaddr* sa6 = addr6.getSockAddr();
    EXPECT_EQ(sa4->sa_family, AF_INET);
    EXPECT_EQ(sa6->sa_family, AF_INET6);
    EXPECT_EQ(reinterpret_cast<const struct sockaddr_in*>(sa4)->sin_port, htons(8080));
    EXPECT_EQ(reinterpret_cast<const struct sockaddr_in6*>(sa6)->sin6_port, htons(8080));
}

// 端口转换正确性
TEST(InetAddressTest, PortEndianConversion) {
    InetAddress addr4("192.168.1.1", 8080);
    InetAddress addr6("::1", 9090, true);

    EXPECT_EQ(addr4.portNetEndian(), htons(8080));
    EXPECT_EQ(addr6.portNetEndian(), htons(9090));
}


// 端口转换正确性
TEST(InetAddressTest, GetAddrLen) {
    InetAddress addr4("192.168.1.1", 8080);
    InetAddress addr6("::1", 9090, true);

    EXPECT_EQ(addr4.getAddrLen(), sizeof(struct sockaddr_in));
    EXPECT_EQ(addr6.getAddrLen(), sizeof(struct sockaddr_in6));
}
