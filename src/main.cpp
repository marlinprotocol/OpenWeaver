#include "GenericRelay.hpp"
#include "Client.hpp"

using namespace marlin::net;
using namespace marlin::beacon;
using namespace marlin::pubsub;

#define MASTER_PUBSUB_PROTOCOL_NUMBER 0x10000001
#define RELAY_PUBSUB_PROTOCOL_NUMBER 0x10000000

int main() {
	GenericRelay<true, true, true> master1(
		MASTER_PUBSUB_PROTOCOL_NUMBER,
		4000,
		SocketAddress::from_string("0.0.0.0:4000"),
		SocketAddress::from_string("0.0.0.0:4002"),
		SocketAddress::from_string("127.0.0.1:8002")
	);

	GenericRelay<true, true, true> master2(
		MASTER_PUBSUB_PROTOCOL_NUMBER,
		4100,
		SocketAddress::from_string("0.0.0.0:4100"),
		SocketAddress::from_string("0.0.0.0:4102"),
		SocketAddress::from_string("127.0.0.1:8002")
	);

	GenericRelay<true, true, true> slave1(
		RELAY_PUBSUB_PROTOCOL_NUMBER,
		5000,
		SocketAddress::from_string("0.0.0.0:5000"),
		SocketAddress::from_string("0.0.0.0:5002"),
		SocketAddress::from_string("127.0.0.1:8002")
	);

	GenericRelay<true, true, true> slave2(
		RELAY_PUBSUB_PROTOCOL_NUMBER,
		5100,
		SocketAddress::from_string("0.0.0.0:5100"),
		SocketAddress::from_string("0.0.0.0:5102"),
		SocketAddress::from_string("127.0.0.1:8002")
	);

	Client client1(
		6000,
		SocketAddress::from_string("0.0.0.0:6000"),
		SocketAddress::from_string("0.0.0.0:6002"),
		SocketAddress::from_string("127.0.0.1:8002")
	);

	Client client2(
		6100,
		SocketAddress::from_string("0.0.0.0:6100"),
		SocketAddress::from_string("0.0.0.0:6102"),
		SocketAddress::from_string("127.0.0.1:8002")
	);

	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
