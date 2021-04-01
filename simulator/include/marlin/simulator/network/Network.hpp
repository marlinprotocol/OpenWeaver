#ifndef MARLIN_SIMULATOR_NETWORK_NETWORK_HPP
#define MARLIN_SIMULATOR_NETWORK_NETWORK_HPP

#include "marlin/simulator/network/DataOnInterfaceEvent.hpp"
#include "marlin/simulator/network/NetworkConditioner.hpp"
#include <marlin/utils/logs.hpp>

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
	
	std::function<void(core::Buffer &)> edit_packet;

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

	if(should_drop) {
		return -2;
	}

	edit_packet(packet);
	std::optional<uint8_t> packet_type = packet.read_uint8_unsafe(1);
	std::optional<uint32_t> s_id = packet.read_uint32(2);
	std::optional<uint32_t> d_id = packet.read_uint32_le(6);
	std::optional<uint64_t> packet_number;
	MARLIN_LOG_INFO("{} {} src {}", conditioner.count, packet_type.value(), src_addr.to_string());
	if (packet_type.value() == 8)
		MARLIN_LOG_DEBUG("Flush Packet {}", conditioner.count);
	if(should_drop) {

		MARLIN_LOG_DEBUG("Packet Dropped {}, {}", packet.size(), packet_type.value());

		if ( packet_type.value() == 2 ) {
			// ACK Packet number read
			packet_number = packet.read_uint64(10);
			MARLIN_LOG_DEBUG(" Src {}, Dst {}, Packet number {}", s_id.value(), d_id.value(), packet_number.value());
		} else if( packet_type.value() == 0 ) {
			// DATA Packet number read
			packet_number = packet.read_uint64(12);
			MARLIN_LOG_DEBUG("DATA {}", packet_number.value());
		}	
		return -2;
	}
	
	auto out_tick = conditioner.get_out_tick(
		manager.current_tick(),
		src_addr,
		dst_addr,
		packet.size()
	);

	auto event = new DataOnInterfaceEvent<
		EventManager,
		core::Buffer,
		NetworkInterface<SelfType>
	>(
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
