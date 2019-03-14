#include "Beacon.hpp"
#include <spdlog/spdlog.h>

namespace marlin {
namespace beacon {

Beacon::Beacon(const net::SocketAddress &_addr) : Node(_addr) {
	DiscoveryProtocol<Beacon>::setup(*this);
}

void Beacon::start_discovery(const net::SocketAddress &addr) {
	DiscoveryProtocol<Beacon>::send_DISCOVER(*this, addr);
}

// DiscoveryProtocol

void Beacon::add_or_update_receipt_time(const net::SocketAddress &addr) {
	if(addr == this->addr) return;

	std::list<Peer>::iterator iter = std::find(peers.begin(), peers.end(), addr);
	if(iter == peers.end()) {
		iter = peers.insert(iter, Peer(addr, std::time(NULL)));
		spdlog::info("New peer: {}", addr.to_string());
	}
	else {
		iter->last_receipt_time = std::time(NULL);
		spdlog::debug("Old peer: {}", addr.to_string());
	}
}

} // namespace beacon
} // namespace marlin
