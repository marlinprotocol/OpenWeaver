#include <iostream>

#include <marlin/beacon/DiscoveryServer.hpp>
#include <marlin/beacon/ClusterDiscoverer.hpp>
#include <marlin/asyncio/core/EventLoop.hpp>
#include <marlin/asyncio/udp/UdpFiber.hpp>
#include <marlin/core/fibers/VersioningFiber.hpp>
#include <marlin/core/fabric/Fabric.hpp>
#include <cstring>
#include <fstream>
#include <boost/filesystem.hpp>

#include <spdlog/fmt/bin_to_hex.h>
#include <structopt/app.hpp>


using namespace marlin;

class BeaconDelegate {
public:
	std::vector<std::tuple<uint32_t, uint16_t, uint16_t>> get_protocols() {
		return {};
	}

	void new_peer_protocol(
		std::array<uint8_t, 20> client_key,
		core::SocketAddress const& addr,
		uint8_t const* static_pk,
		uint32_t protocol,
		uint16_t version
	) {
		if(client_key == std::array<uint8_t, 20>({})) {
			// Skip zero
			return;
		}
		SPDLOG_INFO(
			"New peer: 0x{:spn}, {}, {:spn}, {}, {}",
			spdlog::to_hex(client_key.data(), client_key.data()+20),
			addr.to_string(),
			spdlog::to_hex(static_pk, static_pk+32),
			protocol,
			version
		);
	}

	void new_cluster(
		core::SocketAddress const& addr,
		std::array<uint8_t, 20> const& client_key
	) {
		if(client_key == std::array<uint8_t, 20>({})) {
			// Skip zero
			return;
		}
		SPDLOG_INFO(
			"New cluster: 0x{:spn}, {}",
			spdlog::to_hex(client_key.data(), client_key.data()+20),
			addr.to_string()
		);
	}
};

struct CliOptions {
	std::optional<std::string> discovery_addr;
	std::optional<std::string> bootstrap_addr;
};
STRUCTOPT(CliOptions, discovery_addr, bootstrap_addr);

int main(int argc, char** argv) {
	try {
		auto options = structopt::app("bridge").parse<CliOptions>(argc, argv);
		auto baddr = core::SocketAddress::from_string(
			options.bootstrap_addr.value_or("34.82.79.68:8002")
		);
		auto caddr = core::SocketAddress::from_string(
			options.discovery_addr.value_or("0.0.0.0:18002")
		);

		uint8_t static_sk[crypto_box_SECRETKEYBYTES];
		uint8_t static_pk[crypto_box_PUBLICKEYBYTES];
		crypto_box_keypair(static_pk, static_sk);

		BeaconDelegate del;

		auto crawler = new beacon::ClusterDiscoverer<
			BeaconDelegate
		>(caddr, static_sk);
		crawler->delegate = &del;

		crawler->start_discovery(baddr);

		return asyncio::EventLoop::run();
	} catch (structopt::exception& e) {
		SPDLOG_ERROR("{}", e.what());
		SPDLOG_ERROR("{}", e.help());
	}

	return -1;
}
