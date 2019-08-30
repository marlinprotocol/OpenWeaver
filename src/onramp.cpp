#include "OnRamp.hpp"


using namespace marlin::net;
using namespace marlin::beacon;
using namespace marlin::pubsub;
using namespace marlin::rlpx;

int main() {
	OnRamp onramp;

	OnRampPubSub<OnRamp> ps(SocketAddress::from_string("0.0.0.0:8000"));
	ps.delegate = &onramp;
	onramp.ps = &ps;

	DiscoveryClient<OnRamp> b(SocketAddress::from_string("0.0.0.0:8002"));
	b.delegate = &onramp;
	onramp.b = &b;

	b.start_discovery(SocketAddress::from_string("18.224.44.185:8002"));

	RlpxTransportFactory<OnRamp, OnRamp> f;
	f.bind(SocketAddress::loopback_ipv4(9000));
	f.listen(onramp);

	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
