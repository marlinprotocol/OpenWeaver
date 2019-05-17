#ifndef MARLIN_BEACON_PEER_HPP
#define MARLIN_BEACON_PEER_HPP

#include <ctime>
#include <marlin/net/SocketAddress.hpp>

namespace marlin {
namespace beacon {

struct Peer {
	net::SocketAddress addr;
	std::time_t last_receipt_time;

	Peer(const net::SocketAddress &_addr, const std::time_t &_time) :
		addr(_addr), last_receipt_time(_time) {}

	bool operator==(const Peer &other) const {
		return this->addr == other.addr;
	}

	bool operator==(const net::SocketAddress &addr) const {
		return this->addr == addr;
	}
};

} // namespace beacon
} // namespace marlin

#endif // MARLIN_BEACON_PEER_HPP
