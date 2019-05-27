#include "gtest/gtest.h"
#include "Socket.hpp"


using namespace marlin::net;

TEST(SocketTest, CanBind) {
	Socket s;

	auto res = s.bind(SocketAddress::loopback_ipv4(8000));

	EXPECT_EQ(res, 0);
}
