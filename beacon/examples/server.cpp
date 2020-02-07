#include <marlin/beacon/DiscoveryServer.hpp>
#include <marlin/net/core/EventLoop.hpp>
#include <cstring>

using namespace marlin;

class BeaconDelegate {};

int main() {
	auto baddr = net::SocketAddress::from_string("0.0.0.0:8002");
	BeaconDelegate del;

	auto b = new beacon::DiscoveryServer<BeaconDelegate>(baddr);
	b->delegate = &del;

	return net::EventLoop::run();
}
