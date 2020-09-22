#include <marlin/beacon/DiscoveryServer.hpp>
#include <marlin/asyncio/core/EventLoop.hpp>
#include <cstring>
#include <structopt/app.hpp>

using namespace marlin;

class BeaconDelegate {};

struct CliOptions {
	std::string discovery_addr;
	std::string heartbeat_addr;
};
STRUCTOPT(CliOptions, discovery_addr, heartbeat_addr);

int main(int argc, char** argv) {
	try {
		auto options = structopt::app("beacon").parse<CliOptions>(argc, argv);
		auto discovery_addr = core::SocketAddress::from_string(options.discovery_addr);
		auto heartbeat_addr = core::SocketAddress::from_string(options.heartbeat_addr);

		SPDLOG_INFO(
			"Starting beacon with discovery: {}, heartbeat: {}",
			discovery_addr.to_string(),
			heartbeat_addr.to_string()
		);

		BeaconDelegate del;
		auto b = new beacon::DiscoveryServer<BeaconDelegate>(discovery_addr, heartbeat_addr);
		b->delegate = &del;

		return asyncio::EventLoop::run();
	} catch (structopt::exception& e) {
		SPDLOG_ERROR("{}", e.what());
		SPDLOG_ERROR("{}", e.help());
	}

	return -1;
}
