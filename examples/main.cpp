#include <iostream>

#include "Beacon.hpp"
#include <uv.h>
#include <cstring>

using namespace marlin;

int main() {
	auto addr = net::SocketAddress::from_string("127.0.0.1:8000");
	auto baddr = net::SocketAddress::from_string("127.0.0.1:8000");

	auto b = new beacon::Beacon(addr);
	b->start_listening();

	b->start_discovery(baddr);

	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
