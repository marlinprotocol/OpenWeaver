#include <gtest/gtest.h>

#include <marlin/core/Buffer.hpp>
#include <marlin/core/SocketAddress.hpp>
#include <marlin/core/fibers/VersioningFiber.hpp>
#include <marlin/beacon/Messages.hpp>

#include <cstring>
#include <utility>
#include <vector>

using namespace marlin::core;
using namespace marlin::beacon;

namespace {

// Message type of the discovery fibers
using Msg = VersionedMessage<Buffer>;
using LISTPROTO = LISTPROTOWrapper<Msg>;
using LISTPEER = LISTPEERWrapper<Msg>;
using HEARTBEAT = HEARTBEATWrapper<Msg>;
using LISTCLUSTER = LISTCLUSTERWrapper<Msg>;
using LISTCLUSTER2 = LISTCLUSTER2Wrapper<Msg>;

// A received datagram: version byte, type byte, then size-1 more payload bytes.
// The buffer is exactly as long as the datagram.
Msg datagram(uint8_t type, size_t size) {
	Buffer buf(1 + size);
	std::memset(buf.data(), 0xee, buf.size());
	buf.data()[0] = 0;
	if(size > 0) {
		buf.data()[1] = type;
	}
	return Msg(std::move(buf));
}

// Counts the entries between two iterators, gives up after limit steps
template<typename It>
size_t count(It begin, It end, size_t limit = 100000) {
	size_t n = 0;
	while(begin != end && n < limit) {
		(void)*begin;
		++begin;
		n++;
	}
	return n;
}

std::array<uint8_t, 32> key(uint8_t fill) {
	std::array<uint8_t, 32> k;
	k.fill(fill);
	return k;
}

}

TEST(LISTPEER, EntrySize) {
	EXPECT_EQ(LISTPEER::entry_size, 40u);
}

TEST(LISTPEER, ValidateAcceptsWholeEntriesOnly) {
	// payload size -> valid
	std::vector<std::pair<size_t, bool>> cases = {
		{0, false},
		// no entries
		{1, true},
		{2, false},
		// 8n + 1, accepted by the old check
		{9, false},
		{17, false},
		{25, false},
		{33, false},
		{40, false},
		// one entry
		{41, true},
		{42, false},
		{49, false},
		{80, false},
		// two entries
		{81, true},
		{82, false},
		{121, true},
		// what the beacon sends at most: 30 entries
		{1201, true},
		{1209, false},
		{1400, false},
		// 35 entries
		{1401, true},
		{1409, false},
	};

	for(auto [size, valid] : cases) {
		LISTPEER packet(datagram(3, size));
		EXPECT_EQ(packet.validate(), valid) << "payload of " << size << " bytes";
	}
}

TEST(LISTPEER, IterationStaysInsideMalformedPackets) {
	// Whatever validate says, iteration ends and only reads whole entries
	for(size_t size = 0; size <= 250; size++) {
		LISTPEER packet(datagram(3, size));
		auto n = count(packet.peers_begin(), packet.peers_end());

		auto expected = size < 1 ? 0 : (size - 1) / 40;
		EXPECT_EQ(n, expected) << "payload of " << size << " bytes";
		if(n > 0) {
			EXPECT_LE(1 + n * 40, size) << "payload of " << size << " bytes";
		}
	}
}

TEST(LISTPEER, RoundTrip) {
	std::vector<std::pair<SocketAddress, std::array<uint8_t, 32>>> peers = {
		{SocketAddress::from_string("192.168.0.1:8000"), key(1)},
		{SocketAddress::from_string("10.1.2.3:5002"), key(2)},
		{SocketAddress::from_string("127.0.0.1:1"), key(3)},
	};

	auto begin = peers.begin();
	LISTPEER packet = LISTPEER(150).set_peers(begin, peers.end());
	EXPECT_EQ(begin, peers.end());
	EXPECT_EQ(packet.base.payload_buffer().size(), 1 + 3 * 40u);
	EXPECT_EQ(packet.base.payload_buffer().read_uint8_unsafe(0), 3);
	EXPECT_TRUE(packet.validate());

	size_t i = 0;
	for(auto iter = packet.peers_begin(); iter != packet.peers_end(); ++iter, i++) {
		ASSERT_LT(i, peers.size());
		auto [addr, k] = *iter;
		EXPECT_EQ(addr, peers[i].first);
		EXPECT_EQ(k, peers[i].second);
	}
	EXPECT_EQ(i, 3u);
}

TEST(LISTPEER, FullMessageValidates) {
	// More peers than fit, as the beacon does with LISTPEER(150)
	std::vector<std::pair<SocketAddress, std::array<uint8_t, 32>>> peers;
	for(uint8_t i = 0; i < 100; i++) {
		peers.push_back({SocketAddress::loopback_ipv4(8000 + i), key(i)});
	}

	auto begin = peers.begin();
	LISTPEER packet = LISTPEER(150).set_peers(begin, peers.end());
	// 1 + 8*150 bytes hold 30 entries of 40 bytes
	EXPECT_EQ(begin - peers.begin(), 30);
	EXPECT_EQ(packet.base.payload_buffer().size(), 1201u);
	EXPECT_TRUE(packet.validate());
	EXPECT_EQ(count(packet.peers_begin(), packet.peers_end()), 30u);
}

TEST(LISTPEER, EmptyMessage) {
	std::vector<std::pair<SocketAddress, std::array<uint8_t, 32>>> peers;
	auto begin = peers.begin();
	LISTPEER packet = LISTPEER(150).set_peers(begin, peers.end());

	EXPECT_EQ(packet.base.payload_buffer().size(), 1u);
	EXPECT_TRUE(packet.validate());
	EXPECT_EQ(count(packet.peers_begin(), packet.peers_end()), 0u);
}

TEST(LISTCLUSTER, ValidateAndIteration) {
	for(size_t size = 0; size <= 100; size++) {
		LISTCLUSTER packet(datagram(8, size));
		EXPECT_EQ(packet.validate(), size >= 1 && (size - 1) % 8 == 0) << "payload of " << size << " bytes";

		auto n = count(packet.clusters_begin(), packet.clusters_end());
		EXPECT_EQ(n, size < 1 ? 0 : (size - 1) / 8) << "payload of " << size << " bytes";
	}
}

TEST(LISTCLUSTER, RoundTrip) {
	std::vector<SocketAddress> clusters = {
		SocketAddress::from_string("192.168.0.1:8000"),
		SocketAddress::from_string("10.1.2.3:5002"),
	};

	auto begin = clusters.begin();
	LISTCLUSTER packet = LISTCLUSTER(150).set_clusters(begin, clusters.end());
	EXPECT_TRUE(packet.validate());

	size_t i = 0;
	for(auto iter = packet.clusters_begin(); iter != packet.clusters_end(); ++iter, i++) {
		ASSERT_LT(i, clusters.size());
		EXPECT_EQ(*iter, clusters[i]);
	}
	EXPECT_EQ(i, 2u);
}

TEST(LISTCLUSTER2, ValidateAndIteration) {
	EXPECT_EQ(LISTCLUSTER2::entry_size, 28u);

	for(size_t size = 0; size <= 200; size++) {
		LISTCLUSTER2 packet(datagram(11, size));
		EXPECT_EQ(packet.validate(), size >= 1 && (size - 1) % 28 == 0) << "payload of " << size << " bytes";

		auto n = count(packet.clusters_begin(), packet.clusters_end());
		EXPECT_EQ(n, size < 1 ? 0 : (size - 1) / 28) << "payload of " << size << " bytes";
	}
}

TEST(LISTCLUSTER2, RoundTrip) {
	std::array<uint8_t, 20> a1, a2;
	a1.fill(0x11);
	a2.fill(0x22);
	std::vector<std::tuple<SocketAddress, std::array<uint8_t, 20>>> clusters = {
		{SocketAddress::from_string("192.168.0.1:8000"), a1},
		{SocketAddress::from_string("10.1.2.3:5002"), a2},
	};

	auto begin = clusters.begin();
	LISTCLUSTER2 packet = LISTCLUSTER2(50).set_clusters(begin, clusters.end());
	EXPECT_EQ(packet.base.payload_buffer().size(), 1 + 2 * 28u);
	EXPECT_TRUE(packet.validate());

	size_t i = 0;
	for(auto iter = packet.clusters_begin(); iter != packet.clusters_end(); ++iter, i++) {
		ASSERT_LT(i, clusters.size());
		EXPECT_EQ(*iter, clusters[i]);
	}
	EXPECT_EQ(i, 2u);
}

TEST(LISTPROTO, ValidateAndIteration) {
	// payload size, number of entries claimed in the packet
	for(size_t size = 0; size <= 60; size++) {
		for(size_t claimed : {0, 1, 2, 3, 7, 255}) {
			auto msg = datagram(1, size);
			if(size >= 2) {
				msg.base.data()[2] = (uint8_t)claimed;
			}
			LISTPROTO packet(std::move(msg));

			bool valid = size >= 2 && size >= 2 + claimed * 8;
			EXPECT_EQ(packet.validate(), valid) << "payload of " << size << " bytes, " << claimed << " entries";

			// Never more entries than the buffer holds
			size_t held = size < 2 ? 0 : (size - 2) / 8;
			auto n = count(packet.protocols_begin(), packet.protocols_end());
			EXPECT_EQ(n, std::min(claimed, held)) << "payload of " << size << " bytes, " << claimed << " entries";
		}
	}
}

TEST(LISTPROTO, RoundTrip) {
	std::vector<std::tuple<uint32_t, uint16_t, uint16_t>> protocols = {
		{0x10000000, 1, 8000},
		{0x10000001, 2, 9000},
	};

	LISTPROTO packet = LISTPROTO(protocols.size()).set_protocols(protocols.begin(), protocols.end());
	EXPECT_TRUE(packet.validate());

	size_t i = 0;
	for(auto iter = packet.protocols_begin(); iter != packet.protocols_end(); ++iter, i++) {
		ASSERT_LT(i, protocols.size());
		EXPECT_EQ(*iter, protocols[i]);
	}
	EXPECT_EQ(i, 2u);
}

TEST(HEARTBEAT, Validate) {
	for(size_t size = 0; size <= 40; size++) {
		HEARTBEAT packet(datagram(4, size));
		EXPECT_EQ(packet.validate(), size >= 33) << "payload of " << size << " bytes";
	}
}
