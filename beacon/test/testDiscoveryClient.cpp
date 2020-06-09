#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <marlin/mtest/NetworkFixture.hpp>

#include <marlin/beacon/DiscoveryClient.hpp>


#include <cstring>

using namespace marlin::core;
using namespace marlin::asyncio;
using namespace marlin::simulator;
using namespace marlin::beacon;
using namespace marlin::mtest;


struct Delegate {
	std::function<
		std::vector<std::tuple<uint32_t, uint16_t, uint16_t>>()
	> get_protocols = []() -> std::vector<std::tuple<uint32_t, uint16_t, uint16_t>> {
		return {};
	};

	std::function<void(SocketAddress const&, uint8_t const*, uint32_t, uint16_t)> new_peer = [](
		SocketAddress const& addr,
		uint8_t const* static_pk,
		uint32_t protocol,
		uint16_t version
	) {
		SPDLOG_INFO(
			"New peer: {}, {:spn}, {}, {}",
			addr.to_string(),
			spdlog::to_hex(static_pk, static_pk+32),
			protocol,
			version
		);
	};
};

TEST(DiscoveryClientTest, Constructible) {
	uint8_t static_sk[crypto_box_SECRETKEYBYTES];
	uint8_t static_pk[crypto_box_PUBLICKEYBYTES];
	crypto_box_keypair(static_pk, static_sk);

	DiscoveryClient<Delegate> client(SocketAddress::loopback_ipv4(8000), static_sk);
}


template<typename NetworkInterfaceType>
struct Listener final : public NetworkListener<NetworkInterfaceType> {
	std::function<void(
		NetworkInterfaceType&,
		uint16_t,
		SocketAddress const&,
		Buffer&&
	)> t_did_recv = [](
		NetworkInterfaceType&,
		uint16_t,
		SocketAddress const&,
		Buffer&&
	) {};
	std::function<void()> t_did_close = []() {};

	void did_recv(
		NetworkInterfaceType& interface,
		uint16_t port,
		SocketAddress const& addr,
		Buffer&& packet
	) override {
		t_did_recv(interface, port, addr, std::move(packet));
	}

	void did_close() override {
		t_did_close();
	}
};

TEST_F(DefaultNetworkFixture, DiscoversPeers) {
	uint8_t static_sk[crypto_box_SECRETKEYBYTES];
	uint8_t static_pk[crypto_box_PUBLICKEYBYTES];
	crypto_box_keypair(static_pk, static_sk);

	DiscoveryClient<Delegate, TransportFactoryType, TransportType> client(
		SocketAddress::from_string("192.168.0.1:8002"),
		static_sk,
		i1,
		simulator
	);

	bool listener_called = false;
	ListenerType l;
	l.t_did_recv = [&](
		NetworkInterfaceType& interface,
		uint16_t port,
		SocketAddress const& addr,
		Buffer&& packet
	) {
		listener_called = true;

		// Check src and dst
		EXPECT_EQ(interface.addr.to_string(), "192.168.0.2:0");
		EXPECT_EQ(port, 8002);
		EXPECT_EQ(addr.to_string(), "192.168.0.1:8002");

		// Check packet
		EXPECT_EQ(packet.size(), 2);  // Size
		EXPECT_EQ(packet.read_uint8(0), 0);  // Version
		EXPECT_EQ(packet.read_uint8(1), 2);  // DISCPEER

		if(simulator.current_tick() > 100000) {
			client.close();
		}
	};
	i2.bind(l, 8002);

	client.start_discovery(SocketAddress::from_string("192.168.0.2:8002"));

	simulator.run();

	EXPECT_TRUE(listener_called);
}
