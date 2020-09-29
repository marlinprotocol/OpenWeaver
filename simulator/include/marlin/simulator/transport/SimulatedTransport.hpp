#ifndef MARLIN_SIMULATOR_UDP_SIMULATEDTRANSPORT_HPP
#define MARLIN_SIMULATOR_UDP_SIMULATEDTRANSPORT_HPP

#include "marlin/simulator/network/NetworkInterface.hpp"

#include <marlin/core/Buffer.hpp>
#include <marlin/core/messages/BaseMessage.hpp>
#include <marlin/core/SocketAddress.hpp>
#include <marlin/core/CidrBlock.hpp>
#include <marlin/core/TransportManager.hpp>


namespace marlin {
namespace simulator {

template<
	typename EventManager,
	typename NetworkInterfaceType,
	typename DelegateType
>
class SimulatedTransport {
public:
	using SelfType = SimulatedTransport<
		EventManager,
		NetworkInterfaceType,
		DelegateType
	>;

private:
	NetworkInterfaceType& interface;
	core::TransportManager<SelfType>& transport_manager;

	EventManager& manager;
public:
	using MessageType = core::BaseMessage;

	core::SocketAddress src_addr;
	core::SocketAddress dst_addr;

	bool internal = false;

	DelegateType* delegate = nullptr;

	SimulatedTransport(
		core::SocketAddress const& src_addr,
		core::SocketAddress const& dst_addr,
		NetworkInterfaceType& interface,
		core::TransportManager<SelfType>& transport_manager,
		EventManager& manager
	);

	void setup(DelegateType* delegate);
	void close(uint16_t reason = 0);

	int send(core::Buffer&& buf);
	int send(MessageType&& buf);
	void did_recv(
		core::SocketAddress const& addr,
		core::Buffer&& message
	);

	bool is_internal();
};


// Impl

template<
	typename EventManager,
	typename NetworkInterfaceType,
	typename DelegateType
>
SimulatedTransport<
	EventManager,
	NetworkInterfaceType,
	DelegateType
>::SimulatedTransport(
	core::SocketAddress const& src_addr,
	core::SocketAddress const& dst_addr,
	NetworkInterfaceType& interface,
	core::TransportManager<SelfType>& transport_manager,
	EventManager& manager
) : interface(interface), transport_manager(transport_manager),
	manager(manager), src_addr(src_addr), dst_addr(dst_addr) {
	if(
		core::CidrBlock::from_string("10.0.0.0/8").does_contain_address(dst_addr) ||
		core::CidrBlock::from_string("172.16.0.0/12").does_contain_address(dst_addr) ||
		core::CidrBlock::from_string("192.168.0.0/16").does_contain_address(dst_addr) ||
		core::CidrBlock::from_string("127.0.0.0/8").does_contain_address(dst_addr)
	) {
		internal = true;
	}
}


template<
	typename EventManager,
	typename NetworkInterfaceType,
	typename DelegateType
>
void SimulatedTransport<
	EventManager,
	NetworkInterfaceType,
	DelegateType
>::setup(DelegateType* delegate) {
	this->delegate = delegate;
}

template<
	typename EventManager,
	typename NetworkInterfaceType,
	typename DelegateType
>
void SimulatedTransport<
	EventManager,
	NetworkInterfaceType,
	DelegateType
>::close(uint16_t reason) {
	delegate->did_close(*this, reason);
	transport_manager.erase(dst_addr);
}

template<
	typename EventManager,
	typename NetworkInterfaceType,
	typename DelegateType
>
int SimulatedTransport<
	EventManager,
	NetworkInterfaceType,
	DelegateType
>::send(core::Buffer&& buf) {
	return interface.send(
		manager,
		this->src_addr,
		this->dst_addr,
		std::move(buf)
	);
}

template<
	typename EventManager,
	typename NetworkInterfaceType,
	typename DelegateType
>
int SimulatedTransport<
	EventManager,
	NetworkInterfaceType,
	DelegateType
>::send(MessageType&& buf) {
	return send(std::move(buf).payload_buffer());
}

template<
	typename EventManager,
	typename NetworkInterfaceType,
	typename DelegateType
>
void SimulatedTransport<
	EventManager,
	NetworkInterfaceType,
	DelegateType
>::did_recv(
	core::SocketAddress const&,
	core::Buffer&& message
) {
	delegate->did_recv(*this, std::move(message));
}

template<
	typename EventManager,
	typename NetworkInterfaceType,
	typename DelegateType
>
bool SimulatedTransport<
	EventManager,
	NetworkInterfaceType,
	DelegateType
>::is_internal() {
	return internal;
}

} // namespace simulator
} // namespace marlin

#endif // MARLIN_SIMULATOR_UDP_SIMULATEDTRANSPORT_HPP
