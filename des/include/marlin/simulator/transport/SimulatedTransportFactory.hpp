#ifndef MARLIN_SIMULATOR_UDP_SIMULATEDTRANSPORTFACTORY_HPP
#define MARLIN_SIMULATOR_UDP_SIMULATEDTRANSPORTFACTORY_HPP

#include "marlin/simulator/transport/SimulatedTransport.hpp"

#include <marlin/net/Buffer.hpp>
#include <marlin/net/SocketAddress.hpp>
#include <marlin/net/core/TransportManager.hpp>


namespace marlin {
namespace simulator {

template<
	typename NetworkInterfaceType,
	typename ListenDelegateType,
	typename TransportDelegateType
>
class SimulatedTransportFactory {
public:
	using SelfType = SimulatedTransportFactory<
		NetworkInterfaceType,
		ListenDelegateType,
		TransportDelegateType
	>;

	using TransportType = SimulatedTransport<
		NetworkInterfaceType,
		TransportDelegateType
	>;

private:
	NetworkInterfaceType& interface;
	TransportManager<TransportType>& transport_manager;

	EventManager& manager;
public:
	SocketAddress addr;

	SimulatedTransportFactory(
		NetworkInterfaceType& interface,
		EventManager& manager
	);

	int bind(SocketAddress const& addr);
	int listen(ListenDelegate& delegate);

	int dial(SocketAddress const& addr, ListenDelegate& delegate);
	template<typename MetadataType>
	int dial(SocketAddress const& addr, ListenDelegate& delegate, MetadataType* metadata);
	template<typename MetadataType>
	int dial(SocketAddress const& addr, ListenDelegate& delegate, MetadataType&& metadata);

	TransportType* get_transport(SocketAddress const& addr);
};


//Impl



} // namespace simulator
} // namespace marlin

#endif // MARLIN_SIMULATOR_UDP_SIMULATEDTRANSPORTFACTORY_HPP
