#ifndef MARLIN_BEACON_BEACON_HPP
#define MARLIN_BEACON_BEACON_HPP

#include <marlin/net/Socket.hpp>
#include <marlin/net/Node.hpp>
#include <list>
#include "Peer.hpp"
#include "protocol/DiscoveryProtocol.hpp"

namespace marlin {
namespace beacon {

class Beacon : public net::Node<Beacon, DiscoveryProtocol> {
public:
	Beacon(const net::SocketAddress &addr);

	void start_discovery(const net::SocketAddress &addr);

	// DiscoveryProtocol
	std::list<Peer> peers;

	void add_or_update_receipt_time(const net::SocketAddress &addr);

	uv_timer_t timer;
};

} // namespace beacon
} // namespace marlin

#endif // MARLIN_BEACON_BEACON_HPP
