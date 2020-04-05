#include <marlin/beacon/DiscoveryServer.hpp>
#include <marlin/asyncio/core/EventLoop.hpp>
#include <cstring>

using namespace marlin;

class BeaconDelegate {};

int main() {
	auto baddr = core::SocketAddress::from_string("0.0.0.0:8002");
	BeaconDelegate del;

	auto b = new beacon::DiscoveryServer<BeaconDelegate>(baddr);
	b->delegate = &del;

	return asyncio::EventLoop::run();
}
