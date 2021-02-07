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
		core::SocketAddress,
		NetworkInterface<SelfType>
	> interfaces;

	NetworkConditionerType& conditioner;
public:
	Network(NetworkConditionerType& conditioner);

	NetworkInterface<SelfType>& get_or_create_interface(
		core::SocketAddress const& addr
	);

	template<typename EventManager>
	int send(
		EventManager& manager,
		core::SocketAddress const& src_addr,
		core::SocketAddress const& dst_addr,
		core::Buffer&& packet
	);
};


// Impl

template<typename NetworkConditionerType>
Network<NetworkConditionerType>::Network(
	NetworkConditionerType& conditioner
) : conditioner(conditioner) {}


template<typename NetworkConditionerType>
NetworkInterface<Network<NetworkConditionerType>>& Network<NetworkConditionerType>::get_or_create_interface(
	core::SocketAddress const& addr
) {
	core::SocketAddress taddr = addr;
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
	core::SocketAddress const& src_addr,
	core::SocketAddress const& dst_addr,
	core::Buffer&& packet
) {
	core::SocketAddress taddr = dst_addr;
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

	if (conditioner.count == 27) {
		std::optional<uint32_t> src_conn_id = packet.read_uint32_le(2);
		SPDLOG_INFO("src_conn_id {}",src_conn_id.value());
		int ret = packet.write_uint32_be(2,0);
		ret++;
	}

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
		core::Buffer,
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
