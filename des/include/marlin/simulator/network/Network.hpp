#ifndef MARLIN_SIMULATOR_NETWORK_NETWORK_HPP
#define MARLIN_SIMULATOR_NETWORK_NETWORK_HPP

#include "marlin/net/SocketAddress.hpp"

namespace marlin {
namespace simulator {

class Network {
private:
	std::unordered_map<
		net::SocketAddress,
		NetworkInterface<Network>
	> interfaces;
public:
	NetworkInterface<Network>& get_or_create_interface(
		net::SocketAddress addr
	);

	int send(
		net::SocketAddress const& src_addr,
		net::SocketAddress& dst_addr,
		net::Buffer&& packet
	);
};


// Impl

NetworkInterface<Network>& Network::get_or_create_interface(
	net::SocketAddress addr
) {
	addr.set_port(0);

	return interfaces.try_emplace(
		addr,
		*this,
		addr
	).first->second;
}

int Network::send(
	net::SocketAddress const& src_addr,
	net::SocketAddress& dst_addr,
	net::Buffer&& packet
) {
	auto port = dst_addr.port();
	dst_addr.set_port(0);

	if(interfaces.find(dst_addr) == interfaces.end()) {
		return -1;
	}

	auto interface = interfaces[dst_addr];
	interface.did_recv(port, src_addr, std::move(packet));

	return 0;
}

} // namespace simulator
} // namespace marlin

#endif // MARLIN_SIMULATOR_NETWORK_NETWORK_HPP
