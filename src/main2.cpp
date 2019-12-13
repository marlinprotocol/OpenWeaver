#include "GenericRelay.hpp"
#include "Client.hpp"

using namespace marlin::net;
using namespace marlin::beacon;
using namespace marlin::pubsub;

#define MASTER_PUBSUB_PROTOCOL_NUMBER 0x10000000
#define RELAY_PUBSUB_PROTOCOL_NUMBER 0x10000001
#define CLIENT_PUBSUB_PROTOCOL_NUMBER 0x10000002

int main() {
	GenericRelay<true, true, true> relay1(
		0x10000000,
		SocketAddress::from_string("0.0.0.0:8000"),
		SocketAddress::from_string("0.0.0.0:8002"),
		SocketAddress::from_string("0.0.0.0:8003")
	);

	Client client(
		SocketAddress::from_string("0.0.0.0:9000"),
		SocketAddress::from_string("0.0.0.0:9002"),
		SocketAddress::from_string("0.0.0.0:9003")
	);

	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
