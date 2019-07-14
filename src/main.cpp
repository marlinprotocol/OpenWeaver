#include "Relay.hpp"

using namespace marlin::net;
using namespace marlin::beacon;
using namespace marlin::pubsub;

int main() {
	Relay relay;

	PubSubNode<Relay> ps(SocketAddress::from_string("0.0.0.0:8000"));
	ps.delegate = &relay;
	relay.ps = &ps;

	Beacon<Relay> b(SocketAddress::from_string("0.0.0.0:8002"));
	b.protocol_storage.delegate = &relay;
	relay.b = &b;
	b.start_listening();

	b.start_discovery(SocketAddress::from_string("127.0.0.1:100"));

	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
