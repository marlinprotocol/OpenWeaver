#include "gtest/gtest.h"
#include "protocol/PingProtocol.hpp"

#include "MockTransport.hpp"

using namespace marlin::net;
using namespace marlin::beacon;

TEST(PingProtocolTest, CanSendPING) {
	MockTransport<PingProtocol, PingStorage> transport;
	transport.send = [](Packet &&p, const SocketAddress &addr) {
		EXPECT_EQ(addr, SocketAddress::from_string("192.168.0.1:8000"));

		EXPECT_EQ(p.data()[0], 0);
		EXPECT_EQ(p.data()[1], 0);
	};

	PingProtocol<MockTransport<PingProtocol, PingStorage>>::send_PING(
		transport,
		SocketAddress::from_string("192.168.0.1:8000")
	);
}

TEST(PingProtocolTest, CanSendPONG) {
	MockTransport<PingProtocol, PingStorage> transport;
	transport.send = [](Packet &&p, const SocketAddress &addr) {
		EXPECT_EQ(addr, SocketAddress::from_string("192.168.0.1:8000"));

		EXPECT_EQ(p.data()[0], 0);
		EXPECT_EQ(p.data()[1], 1);
	};

	PingProtocol<MockTransport<PingProtocol, PingStorage>>::send_PONG(
		transport,
		SocketAddress::from_string("192.168.0.1:8000")
	);
}

TEST(PingProtocolTest, DoesSendPONGOnPING) {
	MockTransport<PingProtocol, PingStorage> transport;
	transport.send = [](Packet &&p, const SocketAddress &addr) {
		EXPECT_EQ(addr, SocketAddress::from_string("192.168.0.1:8000"));

		EXPECT_EQ(p.data()[0], 0);
		EXPECT_EQ(p.data()[1], 1);
	};

	PingProtocol<MockTransport<PingProtocol, PingStorage>>::did_receive_PING(
		transport,
		SocketAddress::from_string("192.168.0.1:8000")
	);
}
