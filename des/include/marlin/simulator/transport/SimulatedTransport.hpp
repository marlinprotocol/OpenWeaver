#ifndef MARLIN_SIMULATOR_UDP_SIMULATEDTRANSPORT_HPP
#define MARLIN_SIMULATOR_UDP_SIMULATEDTRANSPORT_HPP

#include "marlin/simulator/network/NetworkInterface.hpp"

#include <marlin/net/Buffer.hpp>
#include <marlin/net/SocketAddress.hpp>
#include <marlin/net/core/TransportManager.hpp>


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
	net::TransportManager<SelfType>& transport_manager;

	EventManager& manager;
public:
	net::SocketAddress src_addr;
	net::SocketAddress dst_addr;

	DelegateType* delegate = nullptr;

	SimulatedTransport(
		net::SocketAddress const& src_addr,
		net::SocketAddress const& dst_addr,
		NetworkInterfaceType& interface,
		net::TransportManager<SelfType>& transport_manager,
		EventManager& manager
	);

	void setup(DelegateType* delegate);
	void close();

	int send(net::Buffer&& buf);
	void did_recv(
		net::SocketAddress const& addr,
		net::Buffer&& message
	);
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
	net::SocketAddress const& src_addr,
	net::SocketAddress const& dst_addr,
	NetworkInterfaceType& interface,
	net::TransportManager<SelfType>& transport_manager,
	EventManager& manager
) : interface(interface), transport_manager(transport_manager),
	manager(manager), src_addr(src_addr), dst_addr(dst_addr) {}


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
>::close() {
	delegate->did_close(*this);
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
>::send(net::Buffer&& buf) {
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
void SimulatedTransport<
	EventManager,
	NetworkInterfaceType,
	DelegateType
>::did_recv(
	net::SocketAddress const&,
	net::Buffer&& message
) {
	delegate->did_recv_packet(*this, std::move(message));
}

} // namespace simulator
} // namespace marlin

#endif // MARLIN_SIMULATOR_UDP_SIMULATEDTRANSPORT_HPP
