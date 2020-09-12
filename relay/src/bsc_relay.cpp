#include "Relay.hpp"
#include <marlin/bsc/Abci.hpp>

#include <structopt/app.hpp>

using namespace marlin::core;
using namespace marlin::asyncio;
using namespace marlin::beacon;
using namespace marlin::pubsub;


struct CliOptions {
	std::string beacon_addr;
	std::string datadir;
	std::optional<uint16_t> pubsub_port;
	std::optional<uint16_t> discovery_port;
};
STRUCTOPT(CliOptions, beacon_addr, datadir, pubsub_port, discovery_port);

int main(int argc, char** argv) {
	try {
		auto options = structopt::app("bsc_relay").parse<CliOptions>(argc, argv);
		auto beacon_addr = options.beacon_addr;
		auto datadir = options.datadir;
		auto pubsub_port = options.pubsub_port.value_or(5000);
		auto discovery_port = options.discovery_port.value_or(5002);

		SPDLOG_INFO(
			"Starting relay on pubsub port: {}, discovery_port: {}, beacon server: {}",
			pubsub_port,
			discovery_port,
			beacon_addr
		);

		Relay<true, true, true, marlin::bsc::Abci> relay(
			RELAY_PUBSUB_PROTOCOL_NUMBER,
			pubsub_port,
			SocketAddress::from_string(std::string("0.0.0.0:").append(std::to_string(pubsub_port))),
			SocketAddress::from_string(std::string("0.0.0.0:").append(std::to_string(discovery_port))),
			SocketAddress::from_string(beacon_addr),
			datadir
		);

		return EventLoop::run();
	} catch (structopt::exception& e) {
		SPDLOG_ERROR("{}", e.what());
		SPDLOG_ERROR("{}", e.help());
	}

	return -1;
}
