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
	BeaconDelegate *delegate;

	std::list<Peer> peers;
	net::SocketAddress beacon_addr;

	void add_or_update_receipt_time(const net::SocketAddress &addr);

	uv_timer_t timer;
};

template<typename BeaconDelegate>
Beacon<BeaconDelegate>::Beacon(const net::SocketAddress &_addr) : BaseNode(_addr) {
	DiscoveryProtocol<Beacon<BeaconDelegate>>::setup(*this);
}

template<typename BeaconDelegate>
void Beacon<BeaconDelegate>::start_discovery(const net::SocketAddress &addr) {
	this->beacon_addr = addr;
	DiscoveryProtocol<Beacon<BeaconDelegate>>::send_DISCOVER(*this, addr);
}

// DiscoveryProtocol

template<typename BeaconDelegate>
void Beacon<BeaconDelegate>::add_or_update_receipt_time(const net::SocketAddress &addr) {
	if(addr == this->addr) return;

	std::list<Peer>::iterator iter = std::find(peers.begin(), peers.end(), addr);
	if(iter == peers.end()) {
		iter = peers.insert(iter, Peer(addr, std::time(NULL)));
		SPDLOG_DEBUG("New peer: {}", addr.to_string());

		delegate->handle_new_peer(addr);
	} else {
		iter->last_receipt_time = std::time(NULL);
		SPDLOG_DEBUG("Old peer: {}", addr.to_string());
	}
}

} // namespace beacon
} // namespace marlin

#endif // MARLIN_BEACON_BEACON_HPP
