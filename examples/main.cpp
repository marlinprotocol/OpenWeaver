#include <iostream>

#include "Beacon.hpp"
#include <uv.h>
#include <cstring>

using namespace marlin;

class BeaconDelegate {
public:
	void handle_new_peer(const net::SocketAddress &addr) {
		SPDLOG_INFO("New peer: {}", addr.to_string());
	}
};

int main() {
	auto addr = net::SocketAddress::from_string("0.0.0.0:8002");
	// auto baddr = net::SocketAddress::from_string("127.0.0.1:8000");

	BeaconDelegate del;

	auto b = new beacon::Beacon<BeaconDelegate>(addr);
	b->delegate = &del;
	b->start_listening();

	// b->start_discovery(baddr);

	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
