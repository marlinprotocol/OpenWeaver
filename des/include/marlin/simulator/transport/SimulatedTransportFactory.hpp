#ifndef MARLIN_SIMULATOR_UDP_SIMULATEDTRANSPORTFACTORY_HPP
#define MARLIN_SIMULATOR_UDP_SIMULATEDTRANSPORTFACTORY_HPP

#include "marlin/simulator/transport/SimulatedTransport.hpp"

#include <marlin/net/Buffer.hpp>
#include <marlin/net/SocketAddress.hpp>
#include <marlin/net/core/TransportManager.hpp>


namespace marlin {
namespace simulator {

template<
	typename EventManager,
	typename MessageType,
	typename ListenDelegateType,
	typename TransportDelegateType
>
class SimulatedTransportFactory {
public:
	using SelfType = SimulatedTransportFactory<
		EventManager,
		MessageType,
		ListenDelegateType,
		TransportDelegateType
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
	SocketAddress addr;

	SimulatedTransport(
		NetworkLinkType& link,
		EventManager& manager
	);

	int bind(SocketAddress const& addr);
	int listen(ListenDelegate& delegate);

	int dial(SocketAddress const& addr, ListenDelegate& delegate);
	template<typename MetadataType>
	int dial(SocketAddress const& addr, ListenDelegate& delegate, MetadataType* metadata);
	template<typename MetadataType>
	int dial(SocketAddress const& addr, ListenDelegate& delegate, MetadataType&& metadata);

	SelfType* get_transport(SocketAddress const& addr);
};


//Impl



} // namespace simulator
} // namespace marlin

#endif // MARLIN_SIMULATOR_UDP_SIMULATEDTRANSPORTFACTORY_HPP
