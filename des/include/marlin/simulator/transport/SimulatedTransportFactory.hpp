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
	TransportManager<TransportType> transport_manager;

	EventManager& manager;

	bool is_listening = false;
	std::pair<TransportType*, int> dial_impl(SocketAddress const& addr, ListenDelegate& delegate);
public:
	SocketAddress addr;

	SimulatedTransportFactory(
		NetworkInterfaceType& interface,
		EventManager& manager
	);

	int bind(SocketAddress const& addr);
	int listen(ListenDelegateType& delegate);

	int dial(SocketAddress const& addr, ListenDelegateType& delegate);
	template<typename MetadataType>
	int dial(SocketAddress const& addr, ListenDelegateType& delegate, MetadataType* metadata);
	template<typename MetadataType>
	int dial(SocketAddress const& addr, ListenDelegateType& delegate, MetadataType&& metadata);

	TransportType* get_transport(SocketAddress const& addr);
};


//Impl

template<
	typename NetworkInterfaceType,
	typename ListenDelegateType,
	typename TransportDelegateType
>
SimulatedTransportFactory<
	NetworkInterfaceType,
	ListenDelegateType,
	TransportDelegateType
>::SimulatedTransportFactory(
	NetworkInterfaceType& interface,
	EventManager& manager
) : interface(interface), manager(manager) {}


template<
	typename NetworkInterfaceType,
	typename ListenDelegateType,
	typename TransportDelegateType
>
int SimulatedTransportFactory<
	NetworkInterfaceType,
	ListenDelegateType,
	TransportDelegateType
>::bind(SocketAddress const& addr) {
	this->addr = addr;
	return 0;
}

template<
	typename NetworkInterfaceType,
	typename ListenDelegateType,
	typename TransportDelegateType
>
int SimulatedTransportFactory<
	NetworkInterfaceType,
	ListenDelegateType,
	TransportDelegateType
>::listen(ListenDelegateType& delegate) {
	is_listening = true;
	return interface.bind(*this, addr.get_port());
}

template<
	typename NetworkInterfaceType,
	typename ListenDelegateType,
	typename TransportDelegateType
>
std::pair<
	SimulatedTransport<
		NetworkInterfaceType,
		TransportDelegateType
	>*,
	int
> SimulatedTransportFactory<
	NetworkInterfaceType,
	ListenDelegateType,
	TransportDelegateType
>::dial_impl(SocketAddress const& addr, ListenDelegateType& delegate) {
	if(!is_listening) {
		auto status = listen(delegate);
		if(status < 0) {
			return {nullptr, status};
		}
	}

	auto [transport, res] = this->transport_manager.get_or_create(
		addr,
		this->addr,
		addr,
		this->interface,
		this->transport_manager,
		this->manager
	);

	return {transport, res ? 1 : 0};
}

template<
	typename NetworkInterfaceType,
	typename ListenDelegateType,
	typename TransportDelegateType
>
int SimulatedTransportFactory<
	NetworkInterfaceType,
	ListenDelegateType,
	TransportDelegateType
>::dial(SocketAddress const& addr, ListenDelegateType& delegate) {
	auto [transport, status] = dial_impl(addr, delegate);

	if(status < 0) {
		return status;
	} else if(status == 1) {
		delegate.did_create_transport(*transport);
	}

	transport->delegate->did_dial(*transport);

	return status;
}

template<
	typename NetworkInterfaceType,
	typename ListenDelegateType,
	typename TransportDelegateType
>
template<typename MetadataType>
int SimulatedTransportFactory<
	NetworkInterfaceType,
	ListenDelegateType,
	TransportDelegateType
>::dial(
	SocketAddress const& addr,
	ListenDelegateType& delegate,
	MetadataType* metadata
) {
	auto [transport, status] = dial_impl(addr, delegate);

	if(status < 0) {
		return status;
	} else if(status == 1) {
		delegate.did_create_transport(*transport, metadata);
	}

	transport->delegate->did_dial(*transport);

	return status;
}

template<
	typename NetworkInterfaceType,
	typename ListenDelegateType,
	typename TransportDelegateType
>
template<typename MetadataType>
int SimulatedTransportFactory<
	NetworkInterfaceType,
	ListenDelegateType,
	TransportDelegateType
>::dial(
	SocketAddress const& addr,
	ListenDelegateType& delegate,
	MetadataType* metadata
) {
	auto [transport, status] = dial_impl(addr, delegate);

	if(status < 0) {
		return status;
	} else if(status == 1) {
		delegate.did_create_transport(*transport, std::move(metadata));
	}

	transport->delegate->did_dial(*transport);

	return status;
}

template<
	typename NetworkInterfaceType,
	typename ListenDelegateType,
	typename TransportDelegateType
>
SimulatedTransport<
	NetworkInterfaceType,
	TransportDelegateType
>* SimulatedTransportFactory<
	NetworkInterfaceType,
	ListenDelegateType,
	TransportDelegateType
>::get_transport(SocketAddress const& addr) {
	return transport_manager.get(addr);
}

} // namespace simulator
} // namespace marlin

#endif // MARLIN_SIMULATOR_UDP_SIMULATEDTRANSPORTFACTORY_HPP
