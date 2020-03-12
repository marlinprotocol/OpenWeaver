#ifndef MARLIN_SIMULATOR_NETWORK_NETWORK_HPP
#define MARLIN_SIMULATOR_NETWORK_NETWORK_HPP

#include "marlin/simulator/network/NetworkConditioner.hpp"

namespace marlin {
namespace simulator {

template<typename NetworkConditionerType = NetworkConditioner>
class Network {
private:
	std::unordered_map<
		net::SocketAddress,
		NetworkInterface<Network>
	> interfaces;

	NetworkConditionerType& conditioner;
public:
	using SelfType = Network<NetworkConditionerType>;

	Network(NetworkConditionerType& conditioner);

	NetworkInterface<SelfType>& get_or_create_interface(
		net::SocketAddress addr
	);

	int send(
		net::SocketAddress const& src_addr,
		net::SocketAddress dst_addr,
		net::Buffer&& packet
	);
};


// Impl

template<typename NetworkConditionerType>
Network<NetworkConditionerType>::Network(
	NetworkConditionerType& conditioner
) : conditioner(conditioner) {}


template<typename NetworkConditionerType>
NetworkInterface<Network<NetworkConditionerType>>& Network<NetworkConditionerType>::get_or_create_interface(
	net::SocketAddress addr
) {
	addr.set_port(0);

	return interfaces.try_emplace(
		addr,
		*this,
		addr
	).first->second;
}

template<typename NetworkConditionerType>
int Network<NetworkConditionerType>::send(
	net::SocketAddress const& src_addr,
	net::SocketAddress dst_addr,
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
