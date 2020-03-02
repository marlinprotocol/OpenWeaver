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
	TransportManager<SelfType> &transport_manager;

public:
	SocketAddress src_addr;
	SocketAddress dst_addr;

	DelegateType* delegate = nullptr;

	SimulatedTransport(
		SocketAddress const& src_addr,
		SocketAddress const& dst_addr,
		NetworkLinkType& link,
		TransportManager<SelfType>& transport_manager
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


} // namespace simulator
} // namespace marlin

#endif // MARLIN_SIMULATOR_UDP_SIMULATEDTRANSPORT_HPP
