#include "gtest/gtest.h"
#include "SocketAddress.hpp"


using namespace marlin::net;

TEST(SocketAddressTest, DefaultConstructible) {
	SocketAddress addr;

	EXPECT_EQ(addr, SocketAddress::from_string("0.0.0.0:0"));
}

TEST(SocketAddressTest, CopyConstructible) {
	auto addr = SocketAddress::from_string("192.168.0.1:8000");

	SocketAddress naddr(addr);

	EXPECT_EQ(naddr, addr);
}

TEST(SocketAddressTest, CopyAssignable) {
	auto addr = SocketAddress::from_string("192.168.0.1:8000");

	SocketAddress naddr; naddr = addr;

	EXPECT_EQ(naddr, addr);
}

TEST(SocketAddressTest, StringConvertible) {
	auto addr = SocketAddress::from_string("192.168.0.1:8000");

	auto str = addr.to_string();

	EXPECT_EQ(str, "192.168.0.1:8000");
}

TEST(SocketAddressTest, LoopbackConstructible) {
	auto addr = SocketAddress::loopback_ipv4(8000);

	EXPECT_EQ(addr, SocketAddress::from_string("127.0.0.1:8000"));
}
