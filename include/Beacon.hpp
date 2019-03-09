#ifndef MARLIN_BEACON_BEACON_HPP
#define MARLIN_BEACON_BEACON_HPP

#include <marlin/net/Socket.hpp>
#include <list>
#include "Peer.hpp"

namespace marlin {
namespace beacon {

class Beacon {
public:
	net::SocketAddress addr;
	net::Socket localSocket;

	Beacon(const net::SocketAddress &_addr);

	void start_listening();

	void did_receive_packet(
		net::Packet &&p,
		const net::SocketAddress &addr
	);

	void send(net::Packet &&p, const net::SocketAddress &addr);

	void did_send_packet(
		net::Packet &&p,
		const net::SocketAddress &addr
	);

	// DiscoveryProtocol
	std::list<Peer> peers;

	void add_or_update_receipt_time(const net::SocketAddress &addr);

	uv_timer_t timer;

	void start_discovery(const net::SocketAddress &addr);
};

} // namespace beacon
} // namespace marlin

#endif // MARLIN_BEACON_BEACON_HPP
