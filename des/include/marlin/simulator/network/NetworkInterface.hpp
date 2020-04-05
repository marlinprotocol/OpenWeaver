#ifndef MARLIN_SIMULATOR_NETWORK_NETWORKINTERFACE_HPP
#define MARLIN_SIMULATOR_NETWORK_NETWORKINTERFACE_HPP

#include "marlin/core/SocketAddress.hpp"
#include "marlin/core/Buffer.hpp"

#include <unordered_map>

namespace marlin {
namespace simulator {

template<typename NetworkInterfaceType>
class NetworkListener {
public:
	virtual void did_recv(
		NetworkInterfaceType& interface,
		uint16_t port,
		core::SocketAddress const& addr,
		core::Buffer&& packet
	) = 0;
	virtual void did_close() = 0;
};

template<typename NetworkType>
class NetworkInterface {
public:
	using SelfType = NetworkInterface<NetworkType>;
	using NetworkListenerType = NetworkListener<SelfType>;

private:
	std::unordered_map<uint16_t, NetworkListenerType*> listeners;
	NetworkType& network;

public:
	core::SocketAddress addr;

	NetworkInterface(
		NetworkType& network,
		core::SocketAddress const& addr
	);

	int bind(NetworkListenerType& listener, uint16_t port);
	void close(uint16_t port);

	template<typename EventManager>
	int send(EventManager& manager, core::SocketAddress const& src_addr, core::SocketAddress const& addr, core::Buffer&& packet);
	void did_recv(uint16_t port, core::SocketAddress const& addr, core::Buffer&& packet);
};


// Impl

template<typename NetworkType>
NetworkInterface<NetworkType>::NetworkInterface(
	NetworkType& network,
	core::SocketAddress const& addr
) : network(network), addr(addr) {}

template<typename NetworkType>
int NetworkInterface<NetworkType>::bind(NetworkListenerType& listener, uint16_t port) {
	if(listeners.find(port) != listeners.end()) {
		return -1;
	}

	listeners[port] = &listener;

	return 0;
}

template<typename NetworkType>
void NetworkInterface<NetworkType>::close(uint16_t port) {
	if(listeners.find(port) == listeners.end()) {
		return;
	}

	auto* listener = listeners[port];
	listeners.erase(port);
	listener->did_close();
}

template<typename NetworkType>
template<typename EventManager>
int NetworkInterface<NetworkType>::send(
	EventManager& manager,
	core::SocketAddress const& src_addr,
	core::SocketAddress const& dst_addr,
	core::Buffer&& packet
) {
	return network.send(manager, src_addr, dst_addr, std::move(packet));
}

template<typename NetworkType>
void NetworkInterface<NetworkType>::did_recv(
	uint16_t port,
	core::SocketAddress const& addr,
	core::Buffer&& packet
) {
	if(listeners.find(port) == listeners.end()) {
		return;
	}

	auto* listener = listeners.at(port);
	listener->did_recv(*this, port, addr, std::move(packet));
}

} // namespace simulator
} // namespace marlin

#endif // MARLIN_SIMULATOR_NETWORK_NETWORKINTERFACE_HPP
