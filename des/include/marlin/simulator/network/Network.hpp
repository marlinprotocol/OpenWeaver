#ifndef MARLIN_SIMULATOR_NETWORK_NETWORK_HPP
#define MARLIN_SIMULATOR_NETWORK_NETWORK_HPP

#include "marlin/simulator/network/DataOnInterfaceEvent.hpp"
#include "marlin/simulator/network/NetworkConditioner.hpp"

namespace marlin {
namespace simulator {

template<typename NetworkConditionerType = NetworkConditioner>
class Network {
public:
	using SelfType = Network<NetworkConditionerType>;
private:
	std::unordered_map<
		net::SocketAddress,
		NetworkInterface<SelfType>
	> interfaces;

	NetworkConditionerType& conditioner;
public:
	Network(NetworkConditionerType& conditioner);

	NetworkInterface<SelfType>& get_or_create_interface(
		net::SocketAddress const& addr
	);

	template<typename EventManager>
	int send(
		EventManager& manager,
		net::SocketAddress const& src_addr,
		net::SocketAddress const& dst_addr,
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
	net::SocketAddress const& addr
) {
	net::SocketAddress taddr = addr;
	taddr.set_port(0);

	return interfaces.try_emplace(
		taddr,
		*this,
		addr
	).first->second;
}

template<typename NetworkConditionerType>
template<typename EventManager>
int Network<NetworkConditionerType>::send(
	EventManager& manager,
	net::SocketAddress const& src_addr,
	net::SocketAddress const& dst_addr,
	net::Buffer&& packet
) {
	net::SocketAddress taddr = dst_addr;
	taddr.set_port(0);

	if(interfaces.find(taddr) == interfaces.end()) {
		return -1;
	}

	auto& interface = interfaces.at(taddr);

	auto should_drop = conditioner.should_drop(
		manager.current_tick(),
		src_addr,
		dst_addr,
		packet.size()
	);

	if(should_drop) {
		return -2;
	}

	auto out_tick = conditioner.get_out_tick(
		manager.current_tick(),
		src_addr,
		dst_addr,
		packet.size()
	);

	auto event = std::make_shared<DataOnInterfaceEvent<
		EventManager,
		net::Buffer,
		NetworkInterface<SelfType>
	>>(
		out_tick,
		dst_addr.get_port(),
		src_addr,
		std::move(packet),
		interface
	);

	manager.add_event(event);

	return 0;
}

} // namespace simulator
} // namespace marlin

#endif // MARLIN_SIMULATOR_NETWORK_NETWORK_HPP
