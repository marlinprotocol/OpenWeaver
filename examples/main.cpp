#include <iostream>

#include "DiscoveryServer.hpp"
#include "DiscoveryClient.hpp"
#include <uv.h>
#include <cstring>

using namespace marlin;

class BeaconDelegate {
public:
	std::vector<std::tuple<uint32_t, uint16_t, uint16_t>> get_protocols() {
		return {
			std::make_tuple(0, 0, 0),
			std::make_tuple(1, 1, 1),
			std::make_tuple(2, 2, 2),
			std::make_tuple(3, 3, 3)
		};
	}

	void new_peer(net::SocketAddress const &addr, uint32_t protocol, uint16_t version) {
		SPDLOG_INFO("New peer: {}, {}, {}", addr.to_string(), protocol, version);
	}
};

int main() {
	auto baddr = net::SocketAddress::from_string("127.0.0.1:8002");
	auto caddr1 = net::SocketAddress::from_string("127.0.0.1:8000");
	auto caddr2 = net::SocketAddress::from_string("127.0.0.1:8001");

	BeaconDelegate del;

	auto b = new beacon::DiscoveryServer<BeaconDelegate>(baddr);
	b->delegate = &del;

	auto c1 = new beacon::DiscoveryClient<BeaconDelegate>(caddr1);
	c1->delegate = &del;
	c1->is_discoverable = true;

	auto c2 = new beacon::DiscoveryClient<BeaconDelegate>(caddr2);
	c2->delegate = &del;
	c2->is_discoverable = true;

	c1->start_discovery(baddr);
	c2->start_discovery(baddr);

	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
