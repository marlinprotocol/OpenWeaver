#include "gtest/gtest.h"
#include "StreamTransportFactory.hpp"
#include <marlin/net/udp/UdpTransportFactory.hpp>

#include "MockTransport.hpp"

using namespace marlin::net;
using namespace marlin::stream;


struct TransportDelegate {
	std::function<void(StreamTransport<TransportDelegate, UdpTransport> &, Buffer &&)> did_recv_bytes;
	std::function<void(StreamTransport<TransportDelegate, UdpTransport> &, Buffer &&)> did_send_bytes;
	std::function<void(StreamTransport<TransportDelegate, UdpTransport> &)> did_dial;
	std::function<void(StreamTransport<TransportDelegate, UdpTransport> &)> did_close;
};

struct ListenDelegate {
	std::function<bool(SocketAddress const &)> should_accept;
	std::function<void(StreamTransport<TransportDelegate, UdpTransport> &)> did_create_transport;
};

TEST(StreamProtocolTest, CanSendDATA) {
	// UdpTransport<StreamTransport<TransportDelegate, UdpTransport>> ut(
	// 	SocketAddress::loopback_ipv4(8000),
	// 	SocketAddress::loopback_ipv4(8001),
	// 	nullptr
	// );

	StreamTransportFactory<
		ListenDelegate,
		TransportDelegate,
		UdpTransportFactory,
		UdpTransport
	> st;

	ListenDelegate ld;

	st.bind(SocketAddress::loopback_ipv4(8000));

	st.listen(ld);
	st.dial(SocketAddress::loopback_ipv4(8001), ld);
}


// TEST(StreamProtocolTest, CanSendDATA) {
// 	MockTransport<StreamStorage> transport;
// 	transport.send = [](Packet &&p, const SocketAddress &addr) {
// 		EXPECT_EQ(addr, SocketAddress::from_string("192.168.0.1:8000"));

// 		EXPECT_EQ(p.data()[0], 0);
// 		EXPECT_EQ(p.data()[1], 0);
// 	};

// 	StreamProtocol<MockTransport<StreamStorage>>::send_data(
// 		transport,
// 		1,
// 		std::unique_ptr<char[]>(new char[10]),
// 		10,
// 		SocketAddress::from_string("192.168.0.1:8000")
// 	);
// }

// TEST(StreamProtocolTest, CanSendACK) {
// 	MockTransport<StreamStorage> transport;
// 	transport.send = [](Packet &&p, const SocketAddress &addr) {
// 		EXPECT_EQ(addr, SocketAddress::from_string("192.168.0.1:8000"));

// 		EXPECT_EQ(p.data()[0], 0);
// 		EXPECT_EQ(p.data()[1], 2);
// 		EXPECT_EQ(p.read_uint16_be(2), 3);
// 		EXPECT_EQ(p.read_uint64_be(4), 10);
// 		EXPECT_EQ(p.read_uint64_be(12), 1);
// 		EXPECT_EQ(p.read_uint64_be(20), 5);
// 		EXPECT_EQ(p.read_uint64_be(28), 5);
// 	};

// 	AckRanges ranges;
// 	ranges.largest = 10;
// 	ranges.ranges.push_back(1);
// 	ranges.ranges.push_back(5);
// 	ranges.ranges.push_back(5);

// 	StreamProtocol<MockTransport<StreamStorage>>::send_ACK(
// 		transport,
// 		SocketAddress::from_string("192.168.0.1:8000"),
// 		ranges
// 	);
// }
