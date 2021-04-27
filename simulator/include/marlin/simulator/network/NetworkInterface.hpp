#ifndef MARLIN_SIMULATOR_NETWORK_NETWORKINTERFACE_HPP
#define MARLIN_SIMULATOR_NETWORK_NETWORKINTERFACE_HPP

#include "marlin/core/SocketAddress.hpp"
#include "marlin/core/Buffer.hpp"

#include <unordered_map>
#include <set>

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

class DropRange {
private:
	std::set<uint16_t> drop_range;
public:
	void add_range(uint16_t start, uint16_t offset, uint16_t interval){
		for( uint16_t count = start; count <= start+offset; count+=interval ){
			drop_range.emplace(count);
		}
	};
	bool in_range(uint16_t packet_num){
		if ( drop_range.find(packet_num) != drop_range.end() ){
			return true;
		}
		return false;
	};
};

template<typename NetworkType>
class NetworkInterface {
public:
	using SelfType = NetworkInterface<NetworkType>;
	using NetworkListenerType = NetworkListener<SelfType>;

private:
	std::unordered_map<uint16_t, NetworkListenerType*> listeners;
	NetworkType& network;
	uint16_t ingress_packet_count=0;
public:
	core::SocketAddress addr;
	DropRange drop_packets;

	NetworkInterface(
		NetworkType& network,
		core::SocketAddress const& addr
	);

	std::function<bool()> drop_pattern = [](){return false;};

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
	this->ingress_packet_count++;
	if(listeners.find(port) == listeners.end()) {
		return;
	}

	bool drop = this->drop_packets.in_range(this->ingress_packet_count);

	if ( drop ){
		return;
	}

	auto* listener = listeners.at(port);
	listener->did_recv(*this, port, addr, std::move(packet));
}

} // namespace simulator
} // namespace marlin

#endif // MARLIN_SIMULATOR_NETWORK_NETWORKINTERFACE_HPP
