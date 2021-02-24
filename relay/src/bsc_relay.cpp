#include "Relay.hpp"
#include "NameGenerator.hpp"
#include <marlin/bsc/Abci.hpp>

#include <structopt/app.hpp>

using namespace marlin::core;
using namespace marlin::asyncio;
using namespace marlin::beacon;
using namespace marlin::pubsub;


struct CliOptions {
	std::string discovery_addrs;
	std::string heartbeat_addrs;
	std::string datadir;
	std::optional<uint16_t> pubsub_port;
	std::optional<uint16_t> discovery_port;
	std::optional<std::string> address;
	std::optional<std::string> name;
	std::optional<std::string> interface;
	std::optional<std::string> keyname;
};
STRUCTOPT(CliOptions, discovery_addrs, heartbeat_addrs, datadir, pubsub_port, discovery_port, address, name, interface, keyname);

int main(int argc, char** argv) {
	try {
		auto options = structopt::app("bsc_relay").parse<CliOptions>(argc, argv);
		auto datadir = options.datadir;
		auto pubsub_port = options.pubsub_port.value_or(5000);
		auto discovery_port = options.discovery_port.value_or(5002);
		auto address = options.address.value_or("0x0000000000000000000000000000000000000000");
		if(address.size() != 42) {
			structopt::details::visitor visitor("bsc_relay", "");
			CliOptions opt = CliOptions();
			visit_struct::for_each(opt, visitor);
			visitor.optional_field_names.push_back("help");
			visitor.optional_field_names.push_back("version");
			throw structopt::exception(std::string("Invalid address"), visitor);
		}
		auto name = options.name.value_or(NameGenerator::generate());

		size_t pos;
		std::vector<SocketAddress> discovery_addrs;
		while((pos = options.discovery_addrs.find(',')) != std::string::npos) {
			discovery_addrs.push_back(SocketAddress::from_string(options.discovery_addrs.substr(0, pos)));
			options.discovery_addrs = options.discovery_addrs.substr(pos+1);
		}
		discovery_addrs.push_back(SocketAddress::from_string(options.discovery_addrs));

		std::vector<SocketAddress> heartbeat_addrs;
		while((pos = options.heartbeat_addrs.find(',')) != std::string::npos) {
			heartbeat_addrs.push_back(SocketAddress::from_string(options.heartbeat_addrs.substr(0, pos)));
			options.heartbeat_addrs = options.heartbeat_addrs.substr(pos+1);
		}
		heartbeat_addrs.push_back(SocketAddress::from_string(options.heartbeat_addrs));

		SPDLOG_INFO(
			"Starting relay named: {} on pubsub port: {}, discovery_port: {}",
			name,
			pubsub_port,
			discovery_port
		);

		Relay<true, true, true, marlin::bsc::Abci> relay(
			MASTER_PUBSUB_PROTOCOL_NUMBER,
			pubsub_port,
			SocketAddress::from_string(options.interface.value_or(std::string("0.0.0.0")).append(":").append(std::to_string(pubsub_port))),
			SocketAddress::from_string(options.interface.value_or(std::string("0.0.0.0")).append(":").append(std::to_string(discovery_port))),
			std::move(discovery_addrs),
			std::move(heartbeat_addrs),
			address,
			name,
			options.keyname,
			datadir
		);

		return EventLoop::run();
	} catch (structopt::exception& e) {
		SPDLOG_ERROR("{}", e.what());
		SPDLOG_ERROR("{}", e.help());
	}

	return -1;
}
