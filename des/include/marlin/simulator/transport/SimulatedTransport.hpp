#ifndef MARLIN_SIMULATOR_UDP_SIMULATEDTRANSPORT_HPP
#define MARLIN_SIMULATOR_UDP_SIMULATEDTRANSPORT_HPP

#include "marlin/simulator/network/NetworkInterface.hpp"

#include <marlin/net/Buffer.hpp>
#include <marlin/net/SocketAddress.hpp>
#include <marlin/net/core/TransportManager.hpp>


namespace marlin {
namespace simulator {

template<
	typename NetworkInterfaceType,
	typename DelegateType
>
class SimulatedTransport {
public:
	using SelfType = SimulatedTransport<
		NetworkInterfaceType,
		DelegateType
	>;

private:
	NetworkInterfaceType& interface;
	TransportManager<SelfType>& transport_manager;

	EventManager& manager;
public:
	SocketAddress src_addr;
	SocketAddress dst_addr;

	DelegateType* delegate = nullptr;

	SimulatedTransport(
		SocketAddress const& src_addr,
		SocketAddress const& dst_addr,
		NetworkInterfaceType& interface,
		TransportManager<SelfType>& transport_manager,
		EventManager& manager
	);

	void setup(DelegateType* delegate);
	void close();

	int send(net::Buffer&& buf);
	void did_recv(
		NetworkInterfaceType& interface,
		uint16_t port,
		SocketAddress const& addr,
		net::Buffer&& message
	);
};


// Impl

template<
	typename NetworkInterfaceType,
	typename DelegateType
>
SimulatedTransport<
	NetworkInterfaceType,
	DelegateType
>::SimulatedTransport(
	SocketAddress const& src_addr,
	SocketAddress const& dst_addr,
	NetworkInterfaceType& interface,
	TransportManager<Self>& transport_manager,
	EventManager& manager
) : interface(interface), transport_manager(transport_manager),
	manager(manager), src_addr(src_addr), dst_addr(dst_addr) {}


template<
	typename NetworkInterfaceType,
	typename DelegateType
>
void SimulatedTransport<
	NetworkInterfaceType,
	DelegateType
>::setup(DelegateType* delegate) {
	this->delegate = delegate;
}

template<
	typename NetworkInterfaceType,
	typename DelegateType
>
int SimulatedTransport<
	NetworkInterfaceType,
	DelegateType
>::send(Buffer&& buf) {
	return interface.send(
		manager,
		this->src_addr,
		this->dst_addr,
		std::move(buf)
	);
}

template<
	typename NetworkInterfaceType,
	typename DelegateType
>
void SimulatedTransport<
	NetworkInterfaceType,
	DelegateType
>::did_recv(
	NetworkInterfaceType&,
	uint16_t,
	net::SocketAddress const& addr,
	net::Buffer&& message
) {
	delegate->did_recv_packet(*this, std::move(message));
}

} // namespace simulator
} // namespace marlin

#endif // MARLIN_SIMULATOR_UDP_SIMULATEDTRANSPORT_HPP
