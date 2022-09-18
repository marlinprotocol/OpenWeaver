#include <marlin/core/Keystore.hpp>
#include <marlin/beacon/DiscoveryServer.hpp>
#include <marlin/asyncio/core/EventLoop.hpp>
#include <cstring>
#include <structopt/app.hpp>

using namespace marlin;

struct BeaconDelegate {
	void new_reg(core::SocketAddress const& addr, beacon::PeerInfo const& peer_info) {
		SPDLOG_INFO(
			"New reg: {}: 0x{:spn}",
			addr.to_string(),
			spdlog::to_hex(peer_info.address.data(), peer_info.address.data()+20)
		);
	}
};

struct CliOptions {
	std::optional<std::string> keystore_path;
	std::optional<std::string> keystore_pass_path;
	std::optional<std::string> discovery_addr;
	std::optional<std::string> heartbeat_addr;
	std::optional<std::string> beacon_addr;
};
STRUCTOPT(CliOptions,  keystore_path, keystore_pass_path, discovery_addr, heartbeat_addr, beacon_addr);


int main(int argc, char** argv) {
	try {
		auto options = structopt::app("beacon").parse<CliOptions>(argc, argv);
		std::string key;
		if(options.beacon_addr.has_value()) {
			if(options.keystore_path.has_value() && options.keystore_pass_path.has_value()) {
				key = marlin::core::get_key(options.keystore_path.value(), options.keystore_pass_path.value());
				if (key.empty()) {
					SPDLOG_ERROR("keystore file error");
					return 1;
				}
			} else {
				SPDLOG_ERROR("require keystore and password file");
				return 1;
			}
		}
		auto discovery_addr = core::SocketAddress::from_string(
			options.discovery_addr.value_or("127.0.0.1:8002")
		);
		auto heartbeat_addr = core::SocketAddress::from_string(
			options.heartbeat_addr.value_or("127.0.0.1:8003")
		);
		auto beacon_addr = options.beacon_addr.has_value() ? std::make_optional(core::SocketAddress::from_string(*options.beacon_addr)) : std::nullopt;

		SPDLOG_INFO(
			"Starting beacon with discovery: {}, heartbeat: {}",
			discovery_addr.to_string(),
			heartbeat_addr.to_string()
		);

		BeaconDelegate del;
		beacon::DiscoveryServer<BeaconDelegate> b(discovery_addr, heartbeat_addr, beacon_addr, key);
		b.delegate = &del;

		return asyncio::EventLoop::run();
	} catch (structopt::exception& e) {
		SPDLOG_ERROR("{}", e.what());
		SPDLOG_ERROR("{}", e.help());
	}

	return -1;
}

