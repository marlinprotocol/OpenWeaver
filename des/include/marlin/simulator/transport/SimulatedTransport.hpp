#ifndef MARLIN_SIMULATOR_UDP_SIMULATEDTRANSPORT_HPP
#define MARLIN_SIMULATOR_UDP_SIMULATEDTRANSPORT_HPP

#include "marlin/simulator/network/NetworkLink.hpp"

#include <marlin/net/Buffer.hpp>
#include <marlin/net/SocketAddress.hpp>
#include <marlin/net/core/TransportManager.hpp>


namespace marlin {
namespace simulator {

template<typename DelegateType>
class SimulatedTransport {
private:
	NetworkLink& link;
public:
	using SelfType = SimulatedTransport<DelegateType>;

	SocketAddress src_addr;
	SocketAddress dst_addr;

	DelegateType* delegate = nullptr;

	SimulatedTransport(
		SocketAddress const &src_addr,
		SocketAddress const &dst_addr,
		NetworkLink& link,
		TransportManager<SelfType> &transport_manager
	);

	void setup(DelegateType *delegate);
	void did_recv_packet(Buffer &&packet);
	int send(Buffer &&packet);
	void close();
};


} // namespace simulator
} // namespace marlin

#endif // MARLIN_SIMULATOR_UDP_SIMULATEDTRANSPORT_HPP
