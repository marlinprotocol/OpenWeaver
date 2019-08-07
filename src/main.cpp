#include "Relay.hpp"

using namespace marlin::net;
using namespace marlin::beacon;
using namespace marlin::pubsub;

int main() {
	Relay relay;

	PubSubNode<Relay> ps(SocketAddress::from_string("0.0.0.0:8000"));
	ps.delegate = &relay;
	ps.should_relay = true;
	relay.ps = &ps;

	DiscoveryClient<Relay> b(SocketAddress::from_string("0.0.0.0:8002"));
	b.delegate = &relay;
	b.is_discoverable = true;
	relay.b = &b;

	b.start_discovery(SocketAddress::from_string("18.224.44.185:8002"));

	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
