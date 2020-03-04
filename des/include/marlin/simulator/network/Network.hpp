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

} // namespace simulator
} // namespace marlin

#endif // MARLIN_SIMULATOR_NETWORK_NETWORK_HPP
