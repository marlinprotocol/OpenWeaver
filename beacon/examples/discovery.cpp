#include <iostream>

#include <marlin/beacon/DiscoveryServer.hpp>
#include <marlin/beacon/DiscoveryClient.hpp>
#include <marlin/asyncio/core/EventLoop.hpp>
#include <cstring>
#include <fstream>
#include <experimental/filesystem>

#include "spdlog/fmt/bin_to_hex.h"

using namespace marlin;

class BeaconDelegate {
public:
	std::vector<std::tuple<uint32_t, uint16_t, uint16_t>> get_protocols() {
		return {
			std::make_tuple(0, 0, 0),
			std::make_tuple(1, 1, 1),
			std::make_tuple(2, 2, 2),
			std::make_tuple(3, 3, 3)
		};
	}

	void new_peer(
		core::SocketAddress const& addr,
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
	}
};

int main() {
	auto baddr = core::SocketAddress::from_string("127.0.0.1:8002");
	auto haddr = core::SocketAddress::from_string("127.0.0.1:8003");
	auto caddr1 = core::SocketAddress::from_string("127.0.0.1:8000");
	auto caddr2 = core::SocketAddress::from_string("127.0.0.1:8001");

	uint8_t static_sk[crypto_box_SECRETKEYBYTES];
	uint8_t static_pk[crypto_box_PUBLICKEYBYTES];

	if(std::experimental::filesystem::exists("./.marlin/keys/static")) {
		std::ifstream sk("./.marlin/keys/static", std::ios::binary);
		if(!sk.read((char *)static_sk, crypto_box_SECRETKEYBYTES)) {
			throw;
		}
		crypto_scalarmult_base(static_pk, static_sk);
	} else {
		crypto_box_keypair(static_pk, static_sk);

		std::experimental::filesystem::create_directories("./.marlin/keys/");
		std::ofstream sk("./.marlin/keys/static", std::ios::binary);

		sk.write((char *)static_sk, crypto_box_SECRETKEYBYTES);
	}

	BeaconDelegate del;

	auto b = new beacon::DiscoveryServer<BeaconDelegate>(baddr, haddr);
	b->delegate = &del;

	auto c1 = new beacon::DiscoveryClient<BeaconDelegate>(caddr1, static_sk);
	c1->delegate = &del;
	c1->is_discoverable = true;

	auto c2 = new beacon::DiscoveryClient<BeaconDelegate>(caddr2, static_sk);
	c2->delegate = &del;
	c2->is_discoverable = true;

	c1->start_discovery({baddr}, {haddr});
	c2->start_discovery({baddr}, {haddr});

	return asyncio::EventLoop::run();
}
