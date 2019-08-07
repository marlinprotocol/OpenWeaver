#include "DiscoveryServer.hpp"
#include <uv.h>
#include <cstring>

using namespace marlin;

class BeaconDelegate {};

int main() {
	auto baddr = net::SocketAddress::from_string("0.0.0.0:8002");
	BeaconDelegate del;

	auto b = new beacon::DiscoveryServer<BeaconDelegate>(baddr);
	b->delegate = &del;

	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
