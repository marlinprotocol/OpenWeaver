#include <iostream>

#include <marlin/beacon/DiscoveryServer.hpp>
#include <marlin/beacon/ClusterDiscoverer.hpp>
#include <marlin/asyncio/core/EventLoop.hpp>
#include <cstring>
#include <fstream>
#include <experimental/filesystem>

#include "spdlog/fmt/bin_to_hex.h"

using namespace marlin;

class BeaconDelegate {
public:
	std::vector<std::tuple<uint32_t, uint16_t, uint16_t>> get_protocols() {
		return {};
	}

	void new_peer(
        core::SocketAddress const& baddr,
		core::SocketAddress const& addr,
		uint8_t const* static_pk,
		uint32_t protocol,
		uint16_t version
	) {
		SPDLOG_INFO(
			"New peer: {}, {}, {:spn}, {}, {}",
            baddr.to_string(),
			addr.to_string(),
			spdlog::to_hex(static_pk, static_pk+32),
			protocol,
			version
		);
	}
};

int main() {
	auto baddr = core::SocketAddress::from_string("34.82.79.68:8002");
	auto caddr = core::SocketAddress::from_string("0.0.0.0:18002");

	uint8_t static_sk[crypto_box_SECRETKEYBYTES];
	uint8_t static_pk[crypto_box_PUBLICKEYBYTES];
    crypto_box_keypair(static_pk, static_sk);

	BeaconDelegate del;

	auto crawler = new beacon::ClusterDiscoverer<BeaconDelegate>(caddr, static_sk);
	crawler->delegate = &del;

	crawler->start_discovery(baddr);

	return asyncio::EventLoop::run();
}
