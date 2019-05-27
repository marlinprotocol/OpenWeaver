#include "gtest/gtest.h"
#include "protocol/DiscoveryProtocol.hpp"

#include "MockTransport.hpp"

using namespace marlin::net;
using namespace marlin::beacon;

struct MockDelegate {
	std::function<void(const SocketAddress)> handle_new_peer;
};

TEST(DiscoveryProtocolTest, CanSendDISCOVER) {
	MockTransport<DiscoveryProtocol, DiscoveryStorage<MockDelegate>> transport;
	transport.send = [](Packet &&p, const SocketAddress &addr) {
		EXPECT_EQ(addr, SocketAddress::from_string("192.168.0.1:8000"));

		EXPECT_EQ(p.data()[0], 0);
		EXPECT_EQ(p.data()[1], 2);
	};

	DiscoveryProtocol<MockTransport<DiscoveryProtocol, DiscoveryStorage<MockDelegate>>>::send_DISCOVER(
		transport,
		SocketAddress::from_string("192.168.0.1:8000")
	);
}

TEST(DiscoveryProtocolTest, CanSendPEERLIST) {
	MockTransport<DiscoveryProtocol, DiscoveryStorage<MockDelegate>> transport;
	transport.send = [](Packet &&p, const SocketAddress &addr) {
		EXPECT_EQ(addr, SocketAddress::from_string("192.168.0.1:8000"));

		EXPECT_EQ(p.data()[0], 0);
		EXPECT_EQ(p.data()[1], 3);
	};

	DiscoveryProtocol<MockTransport<DiscoveryProtocol, DiscoveryStorage<MockDelegate>>>::send_PEERLIST(
		transport,
		SocketAddress::from_string("192.168.0.1:8000")
	);
}

TEST(DiscoveryProtocolTest, DoesSendPEERLISTOnDISCOVER) {
	MockTransport<DiscoveryProtocol, DiscoveryStorage<MockDelegate>> transport;
	transport.send = [](Packet &&p, const SocketAddress &addr) {
		EXPECT_EQ(addr, SocketAddress::from_string("192.168.0.1:8000"));

		EXPECT_EQ(p.data()[0], 0);
		EXPECT_EQ(p.data()[1], 3);
	};

	DiscoveryProtocol<MockTransport<DiscoveryProtocol, DiscoveryStorage<MockDelegate>>>::did_receive_DISCOVER(
		transport,
		SocketAddress::from_string("192.168.0.1:8000")
	);
}
