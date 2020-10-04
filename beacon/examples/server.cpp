#include <marlin/beacon/DiscoveryServer.hpp>
#include <marlin/asyncio/core/EventLoop.hpp>
#include <cstring>
#include <structopt/app.hpp>

using namespace marlin;

class BeaconDelegate {};

struct CliOptions {
	std::optional<std::string> discovery_addr;
	std::optional<std::string> heartbeat_addr;
	std::optional<std::string> beacon_addr;
};
STRUCTOPT(CliOptions, discovery_addr, heartbeat_addr, beacon_addr);

int main(int argc, char** argv) {
	try {
		auto options = structopt::app("beacon").parse<CliOptions>(argc, argv);
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
		beacon::DiscoveryServer<BeaconDelegate> b(discovery_addr, heartbeat_addr, beacon_addr);
		b.delegate = &del;

		return asyncio::EventLoop::run();
	} catch (structopt::exception& e) {
		SPDLOG_ERROR("{}", e.what());
		SPDLOG_ERROR("{}", e.help());
	}

	return -1;
}
