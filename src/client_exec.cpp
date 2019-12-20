#include "Client.hpp"

using namespace marlin::net;
using namespace marlin::beacon;
using namespace marlin::pubsub;

#define CLIENT_PUBSUB_PROTOCOL_NUMBER 0x10000002

int main(int , char **argv) {
	std::string beacon_addr(argv[1]);
	uint32_t pubsub_port = 6000;
	uint32_t discovery_port = 6002;
	SPDLOG_INFO(
		"Starting relay with on pubsub port: {}, discovery_port: {}, beacon server: {}",
		pubsub_port,
		discovery_port,
		beacon_addr);

	Client client1(
		pubsub_port,
		SocketAddress::from_string("0.0.0.0:6000"),
		SocketAddress::from_string("0.0.0.0:6002"),
		SocketAddress::from_string(beacon_addr)
	);

	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
