#ifndef MARLIN_SIMULATOR_UDP_SIMULATEDTRANSPORTFACTORY_HPP
#define MARLIN_SIMULATOR_UDP_SIMULATEDTRANSPORTFACTORY_HPP

#include "marlin/simulator/transport/SimulatedTransport.hpp"

#include <marlin/core/Buffer.hpp>
#include <marlin/core/SocketAddress.hpp>
#include <marlin/core/TransportManager.hpp>

#include <spdlog/spdlog.h>


namespace marlin {
namespace simulator {

template<
	typename EventManager,
	typename NetworkInterfaceType,
	typename ListenDelegateType,
	typename TransportDelegateType
>
class SimulatedTransportFactory : public NetworkListener<NetworkInterfaceType> {
public:
	using SelfType = SimulatedTransportFactory<
		EventManager,
		NetworkInterfaceType,
		ListenDelegateType,
		TransportDelegateType
	>;

	using TransportType = SimulatedTransport<
		EventManager,
		NetworkInterfaceType,
		TransportDelegateType
	>;

private:
	NetworkInterfaceType& interface;
	core::TransportManager<TransportType> transport_manager;

	EventManager& manager;

	ListenDelegateType* delegate;
	bool is_listening = false;
	std::pair<TransportType*, int> dial_impl(core::SocketAddress const& addr, ListenDelegateType& delegate);
public:
	core::SocketAddress addr;

	SimulatedTransportFactory(
		NetworkInterfaceType& interface,
		EventManager& manager
	);

	int bind(core::SocketAddress const& addr);
	int listen(ListenDelegateType& delegate);

	int dial(core::SocketAddress const& addr, ListenDelegateType& delegate);
	template<typename MetadataType>
	int dial(core::SocketAddress const& addr, ListenDelegateType& delegate, MetadataType* metadata);
	template<typename MetadataType>
	int dial(core::SocketAddress const& addr, ListenDelegateType& delegate, MetadataType&& metadata);

	TransportType* get_transport(core::SocketAddress const& addr);

	void did_recv(
		NetworkInterfaceType& interface,
		uint16_t port,
		core::SocketAddress const& addr,
		core::Buffer&& message
	) override;
	void did_close() override {}
};


//Impl

template<
	typename EventManager,
	typename NetworkInterfaceType,
	typename ListenDelegateType,
	typename TransportDelegateType
>
SimulatedTransportFactory<
	EventManager,
	NetworkInterfaceType,
	ListenDelegateType,
	TransportDelegateType
>::SimulatedTransportFactory(
	NetworkInterfaceType& interface,
	EventManager& manager
) : interface(interface), manager(manager) {}


template<
	typename EventManager,
	typename NetworkInterfaceType,
	typename ListenDelegateType,
	typename TransportDelegateType
>
int SimulatedTransportFactory<
	EventManager,
	NetworkInterfaceType,
	ListenDelegateType,
	TransportDelegateType
>::bind(core::SocketAddress const& addr) {
	this->addr = addr;
	return 0;
}

template<
	typename EventManager,
	typename NetworkInterfaceType,
	typename ListenDelegateType,
	typename TransportDelegateType
>
int SimulatedTransportFactory<
	EventManager,
	NetworkInterfaceType,
	ListenDelegateType,
	TransportDelegateType
>::listen(ListenDelegateType& delegate) {
	this->delegate = &delegate;
	is_listening = true;
	return interface.bind(*this, addr.get_port());
}

template<
	typename EventManager,
	typename NetworkInterfaceType,
	typename ListenDelegateType,
	typename TransportDelegateType
>
std::pair<
	SimulatedTransport<
		EventManager,
		NetworkInterfaceType,
		TransportDelegateType
	>*,
	int
> SimulatedTransportFactory<
	EventManager,
	NetworkInterfaceType,
	ListenDelegateType,
	TransportDelegateType
>::dial_impl(core::SocketAddress const& addr, ListenDelegateType& delegate) {
	if(!is_listening) {
		auto status = listen(delegate);
		if(status < 0) {
			SPDLOG_ERROR("SimulatedTransportFactory {}: Listen failure: {}", this->addr.to_string());
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
	typename EventManager,
	typename NetworkInterfaceType,
	typename ListenDelegateType,
	typename TransportDelegateType
>
int SimulatedTransportFactory<
	EventManager,
	NetworkInterfaceType,
	ListenDelegateType,
	TransportDelegateType
>::dial(core::SocketAddress const& addr, ListenDelegateType& delegate) {
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
	typename EventManager,
	typename NetworkInterfaceType,
	typename ListenDelegateType,
	typename TransportDelegateType
>
template<typename MetadataType>
int SimulatedTransportFactory<
	EventManager,
	NetworkInterfaceType,
	ListenDelegateType,
	TransportDelegateType
>::dial(
	core::SocketAddress const& addr,
	ListenDelegateType& delegate,
	MetadataType* metadata
) {
	auto [transport, status] = dial_impl(addr, delegate);

	if(status < 0) {
		return status;
	} else if(status == 1) {
		delegate.did_create_transport(*transport, metadata);
	}

	transport->delegate->did_dial(*transport, metadata);

	return status;
}

template<
	typename EventManager,
	typename NetworkInterfaceType,
	typename ListenDelegateType,
	typename TransportDelegateType
>
template<typename MetadataType>
int SimulatedTransportFactory<
	EventManager,
	NetworkInterfaceType,
	ListenDelegateType,
	TransportDelegateType
>::dial(
	core::SocketAddress const& addr,
	ListenDelegateType& delegate,
	MetadataType&& metadata
) {
	auto [transport, status] = dial_impl(addr, delegate);

	if(status < 0) {
		return status;
	} else if(status == 1) {
		delegate.did_create_transport(*transport);
	}

	transport->delegate->did_dial(*transport, std::move(metadata));

	return status;
}

template<
	typename EventManager,
	typename NetworkInterfaceType,
	typename ListenDelegateType,
	typename TransportDelegateType
>
SimulatedTransport<
	EventManager,
	NetworkInterfaceType,
	TransportDelegateType
>* SimulatedTransportFactory<
	EventManager,
	NetworkInterfaceType,
	ListenDelegateType,
	TransportDelegateType
>::get_transport(core::SocketAddress const& addr) {
	return transport_manager.get(addr);
}

template<
	typename EventManager,
	typename NetworkInterfaceType,
	typename ListenDelegateType,
	typename TransportDelegateType
>
void SimulatedTransportFactory<
	EventManager,
	NetworkInterfaceType,
	ListenDelegateType,
	TransportDelegateType
>::did_recv(
	NetworkInterfaceType&,
	uint16_t,
	core::SocketAddress const& addr,
	core::Buffer&& message
) {
	if(message.size() == 0) {
		return;
	}

	auto* transport = this->transport_manager.get(addr);
	if(transport == nullptr) {
		// Create new transport if permitted
		if(delegate->should_accept(addr)) {
			transport = this->transport_manager.get_or_create(
				addr,
				this->addr,
				addr,
				this->interface,
				this->transport_manager,
				this->manager
			).first;
			delegate->did_create_transport(*transport);
		} else {
			return;
		}
	}

	transport->did_recv(
		addr,
		std::move(message)
	);
}

} // namespace simulator
} // namespace marlin

#endif // MARLIN_SIMULATOR_UDP_SIMULATEDTRANSPORTFACTORY_HPP
