#ifndef MARLIN_BEACON_BEACON_HPP
#define MARLIN_BEACON_BEACON_HPP

#include <marlin/net/Socket.hpp>
#include <marlin/net/Node.hpp>
#include <list>
#include "protocol/Peer.hpp"
#include "protocol/DiscoveryProtocol.hpp"

namespace marlin {
namespace beacon {

template<typename BeaconDelegate>
class Beacon : public net::Node<Beacon<BeaconDelegate>, DiscoveryProtocol> {
public:
	typedef net::Node<Beacon<BeaconDelegate>, DiscoveryProtocol> BaseNode;

	Beacon(const net::SocketAddress &addr);

	void start_discovery(const net::SocketAddress &addr);

	// DiscoveryProtocol
	DiscoveryStorage<BeaconDelegate> protocol_storage;
};

template<typename BeaconDelegate>
Beacon<BeaconDelegate>::Beacon(const net::SocketAddress &_addr) : BaseNode(_addr) {
	DiscoveryProtocol<Beacon<BeaconDelegate>>::setup(*this);
}

template<typename BeaconDelegate>
void Beacon<BeaconDelegate>::start_discovery(const net::SocketAddress &addr) {
	this->protocol_storage.beacon_addr = addr;
	DiscoveryProtocol<Beacon<BeaconDelegate>>::send_DISCOVER(*this, addr);
}

} // namespace beacon
} // namespace marlin

#endif // MARLIN_BEACON_BEACON_HPP
