#ifndef MARLIN_SIMULATOR_UDP_SIMULATEDTRANSPORT_HPP
#define MARLIN_SIMULATOR_UDP_SIMULATEDTRANSPORT_HPP

#include "marlin/simulator/network/NetworkLink.hpp"

#include <marlin/net/Buffer.hpp>
#include <marlin/net/SocketAddress.hpp>
#include <marlin/net/core/TransportManager.hpp>


namespace marlin {
namespace simulator {

template<
	typename EventManager,
	typename MessageType,
	typename DelegateType
>
class SimulatedTransport {
public:
	using SelfType = SimulatedTransport<
		EventManager,
		MessageType,
		DelegateType
	>;
	using NetworkLinkType = NetworkLink<
		EventManager,
		MessageType,
		SelfType,
		SelfType
	>;

private:
	NetworkLinkType& link;
	TransportManager<SelfType>& transport_manager;

	EventManager& manager;
public:
	SocketAddress src_addr;
	SocketAddress dst_addr;

	DelegateType* delegate = nullptr;

	SimulatedTransport(
		SocketAddress const& src_addr,
		SocketAddress const& dst_addr,
		NetworkLinkType& link,
		TransportManager<SelfType>& transport_manager,
		EventManager& manager
	);

	void setup(DelegateType* delegate);
	void close();

	int send(Buffer&& buf);
	void did_recv(
		typename NetworkLinkType::SrcEventType& event,
		typename NetworkLinkType::EventManager& manager,
		typename NetworkLinkType::MessageType&& message,
		NetworkLinkType& link
	);
};


// Impl

template<
	typename EventManager,
	typename MessageType,
	typename DelegateType
>
SimulatedTransport<
	EventManager,
	MessageType,
	DelegateType
>::SimulatedTransport(
	SocketAddress const& src_addr,
	SocketAddress const& dst_addr,
	NetworkLinkType& link,
	TransportManager<Self>& transport_manager,
	EventManager& manager
) : link(link), transport_manager(transport_manager),
	src_addr(src_addr), dst_addr(dst_addr), manager(manager) {}


template<
	typename EventManager,
	typename MessageType,
	typename DelegateType
>
void SimulatedTransport<
	EventManager,
	MessageType,
	DelegateType
>::setup(DelegateType* delegate) {
	this->delegate = delegate;
}

template<
	typename EventManager,
	typename MessageType,
	typename DelegateType
>
int SimulatedTransport<
	EventManager,
	MessageType,
	DelegateType
>::send(Buffer&& buf) {
	if(&link.src == this) {
		link.send_to_dst(manager, manager.current_tick(), std::move(message));
	} else {
		link.send_to_src(manager, manager.current_tick(), std::move(message));
	}

	return 0;
}

template<
	typename EventManager,
	typename MessageType,
	typename DelegateType
>
void SimulatedTransport<
	EventManager,
	MessageType,
	DelegateType
>::did_recv(
	typename NetworkLinkType::SrcEventType&,
	typename NetworkLinkType::EventManager&,
	typename NetworkLinkType::MessageType&& message,
	NetworkLinkType&
) {
	delegate->did_recv_packet(std::move(message));
}

} // namespace simulator
} // namespace marlin

#endif // MARLIN_SIMULATOR_UDP_SIMULATEDTRANSPORT_HPP
