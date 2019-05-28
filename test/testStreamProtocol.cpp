#include "gtest/gtest.h"
#include "protocol/StreamProtocol.hpp"

#include "MockTransport.hpp"

using namespace marlin::net;
using namespace marlin::stream;

TEST(StreamProtocolTest, CanSendDATA) {
	MockTransport<StreamStorage> transport;
	transport.send = [](Packet &&p, const SocketAddress &addr) {
		EXPECT_EQ(addr, SocketAddress::from_string("192.168.0.1:8000"));

		EXPECT_EQ(p.data()[0], 0);
		EXPECT_EQ(p.data()[1], 0);
	};

	StreamProtocol<MockTransport<StreamStorage>>::send_data(
		transport,
		1,
		std::unique_ptr<char[]>(new char[10]),
		10,
		SocketAddress::from_string("192.168.0.1:8000")
	);
}

TEST(StreamProtocolTest, CanSendACK) {
	MockTransport<StreamStorage> transport;
	transport.send = [](Packet &&p, const SocketAddress &addr) {
		EXPECT_EQ(addr, SocketAddress::from_string("192.168.0.1:8000"));

		EXPECT_EQ(p.data()[0], 0);
		EXPECT_EQ(p.data()[1], 2);
		EXPECT_EQ(p.extract_uint16(2), 3);
		EXPECT_EQ(p.extract_uint64(4), 10);
		EXPECT_EQ(p.extract_uint64(12), 1);
		EXPECT_EQ(p.extract_uint64(20), 5);
		EXPECT_EQ(p.extract_uint64(28), 5);
	};

	AckRanges ranges;
	ranges.largest = 10;
	ranges.ranges.push_back(1);
	ranges.ranges.push_back(5);
	ranges.ranges.push_back(5);

	StreamProtocol<MockTransport<StreamStorage>>::send_ACK(
		transport,
		SocketAddress::from_string("192.168.0.1:8000"),
		ranges
	);
}
