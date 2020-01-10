#include "GenericRelay.hpp"
#include "Client.hpp"

using namespace marlin::net;
using namespace marlin::beacon;
using namespace marlin::pubsub;

#define MASTER_PUBSUB_PROTOCOL_NUMBER 0x10000001

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

	GenericRelay<true, true, true> master1(
		MASTER_PUBSUB_PROTOCOL_NUMBER,
		pubsub_port,
		SocketAddress::from_string("0.0.0.0:4000"),
		SocketAddress::from_string("0.0.0.0:4002"),
		SocketAddress::from_string(beacon_addr)
	);

	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
