#include "Relay.hpp"
#include "NameGenerator.hpp"

#include <structopt/app.hpp>

#ifndef MARLIN_RELAY_DEFAULT_PUBSUB_PORT
#define MARLIN_RELAY_DEFAULT_PUBSUB_PORT 15000
#endif

#ifndef MARLIN_RELAY_DEFAULT_DISC_PORT
#define MARLIN_RELAY_DEFAULT_DISC_PORT 15002
#endif

// Pfff, of course macros make total sense!
#define STRH(X) #X
#define STR(X) STRH(X)


using namespace marlin::core;
using namespace marlin::asyncio;
using namespace marlin::beacon;
using namespace marlin::pubsub;


struct CliOptions {
	std::optional<std::string> discovery_addrs;
	std::optional<std::string> heartbeat_addrs;
	std::optional<std::string> discovery_bind_addr;
	std::optional<std::string> pubsub_bind_addr;
	std::optional<std::string> name;
	std::optional<std::string> keyname;
};
STRUCTOPT(CliOptions, discovery_addrs, heartbeat_addrs, discovery_bind_addr, pubsub_bind_addr, name, keyname);

int main(int argc, char** argv) {
	try {
		auto options = structopt::app("relay").parse<CliOptions>(argc, argv);
		auto pubsub_bind_addr = options.pubsub_bind_addr.value_or(
			std::string("0.0.0.0:").append(STR(MARLIN_RELAY_DEFAULT_PUBSUB_PORT))
		);
		auto discovery_bind_addr = options.discovery_bind_addr.value_or(
			std::string("0.0.0.0:").append(STR(MARLIN_RELAY_DEFAULT_DISCOVERY_PORT))
		);
		auto name = options.name.value_or(NameGenerator::generate());

		size_t pos;
		std::string addrs = options.discovery_addrs.value_or("127.0.0.1:8002");
		std::vector<SocketAddress> discovery_addrs;
		while((pos = addrs.find(',')) != std::string::npos) {
			discovery_addrs.push_back(SocketAddress::from_string(addrs.substr(0, pos)));
			addrs = addrs.substr(pos+1);
		}
		discovery_addrs.push_back(SocketAddress::from_string(addrs));

		addrs = options.heartbeat_addrs.value_or("127.0.0.1:8003");
		std::vector<SocketAddress> heartbeat_addrs;
		while((pos = addrs.find(',')) != std::string::npos) {
			heartbeat_addrs.push_back(SocketAddress::from_string(addrs.substr(0, pos)));
			addrs = addrs.substr(pos+1);
		}
		heartbeat_addrs.push_back(SocketAddress::from_string(addrs));

		SPDLOG_INFO(
			"Starting relay named: {} on pubsub addr: {}, discovery addr: {}",
			name,
			pubsub_bind_addr,
			discovery_bind_addr
		);

		Relay<true, true, true> relay(
			MASTER_PUBSUB_PROTOCOL_NUMBER,
			SocketAddress::from_string(pubsub_bind_addr),
			SocketAddress::from_string(discovery_bind_addr),
			std::move(discovery_addrs),
			std::move(heartbeat_addrs),
			"",
			name,
			options.keyname
		);

		return EventLoop::run();
	} catch (structopt::exception& e) {
		SPDLOG_ERROR("{}", e.what());
		SPDLOG_ERROR("{}", e.help());
	}

	return -1;
}
