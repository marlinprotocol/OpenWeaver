#include "Relay.hpp"

using namespace marlin::core;
using namespace marlin::asyncio;
using namespace marlin::beacon;
using namespace marlin::pubsub;


int main(int , char **argv) {
	std::string beacon_addr(argv[1]);
	uint32_t pubsub_port = 4000;
	uint32_t discovery_port = 4002;
	SPDLOG_INFO(
		"Starting master on pubsub port: {}, discovery_port: {}, beacon server: {}",
		pubsub_port,
		discovery_port,
		beacon_addr
	);

	Relay<true, true, true> master1(
		MASTER_PUBSUB_PROTOCOL_NUMBER,
		pubsub_port,
		SocketAddress::from_string("0.0.0.0:4000"),
		SocketAddress::from_string("0.0.0.0:4002"),
		SocketAddress::from_string(beacon_addr)
	);

	return EventLoop::run();
}
