#include "Client.hpp"

using namespace marlin::net;
using namespace marlin::beacon;
using namespace marlin::pubsub;

#define CLIENT_PUBSUB_PROTOCOL_NUMBER 0x10000002

int main(int , char **argv) {
	std::string beacon_addr(argv[1]);
	SPDLOG_INFO("Beacon: {}", beacon_addr);

	Client client1(
		6000,
		SocketAddress::from_string("0.0.0.0:6000"),
		SocketAddress::from_string("0.0.0.0:6002"),
		SocketAddress::from_string(beacon_addr)
	);

	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
