#include "GenericRelay.hpp"
#include "Client.hpp"

using namespace marlin::net;
using namespace marlin::beacon;
using namespace marlin::pubsub;

#define RELAY_PUBSUB_PROTOCOL_NUMBER 0x10000001

int main(int , char **argv) {
	std::string beacon_addr(argv[1]);
	SPDLOG_INFO("Beacon: {}", beacon_addr);

	GenericRelay<true, true, true> master1(
		RELAY_PUBSUB_PROTOCOL_NUMBER,
		5000,
		SocketAddress::from_string("0.0.0.0:5000"),
		SocketAddress::from_string("0.0.0.0:5002"),
		SocketAddress::from_string(beacon_addr)
	);

	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
