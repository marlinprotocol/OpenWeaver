#ifndef MARLIN_SIMULATOR_NETWORK_NETWORKINTERFACE_HPP
#define MARLIN_SIMULATOR_NETWORK_NETWORKINTERFACE_HPP

#include "marlin/net/SocketAddress.hpp"


namespace marlin {
namespace simulator {

class NetworkListener {
	virtual void did_recv(
		net::SocketAddress const& addr,
		net::Buffer&& packet
	) = 0;
	virtual void did_close();
}

template<typename NetworkType>
class NetworkInterface {
private:
	std::unordered_map<uint16_t, NetworkListener*> listeners;
	NetworkType& network;

public:
	net::SocketAddress addr;

	NetworkInterface(
		NetworkType& network,
		net::SocketAddress const& addr
	);

	int bind(NetworkListener& listener, uint16_t port);
	void close(uint16_t port);

	int send(uint16_t port, SocketAddress const& addr, net::Buffer&& packet);
	void did_recv(uint16_t port, net::SocketAddress const& addr, net::Buffer&& packet);
};

} // namespace simulator
} // namespace marlin

#endif // MARLIN_SIMULATOR_NETWORK_NETWORKINTERFACE_HPP
