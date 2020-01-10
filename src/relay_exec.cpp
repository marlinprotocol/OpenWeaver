#include "GenericRelay.hpp"
#include "Client.hpp"

using namespace marlin::net;
using namespace marlin::beacon;
using namespace marlin::pubsub;

#define RELAY_PUBSUB_PROTOCOL_NUMBER 0x10000000

int main(int , char **argv) {
	std::string beacon_addr(argv[1]);
	uint32_t pubsub_port = 5000;
	uint32_t discovery_port = 5002;
	SPDLOG_INFO(
		"Starting relay on pubsub port: {}, discovery_port: {}, beacon server: {}",
		pubsub_port,
		discovery_port,
		beacon_addr
	);

	GenericRelay<true, true, true> master1(
		RELAY_PUBSUB_PROTOCOL_NUMBER,
		pubsub_port,
		SocketAddress::from_string("0.0.0.0:5000"),
		SocketAddress::from_string("0.0.0.0:5002"),
		SocketAddress::from_string(beacon_addr)
	);

	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
