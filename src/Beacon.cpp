#include "Beacon.hpp"
#include "protocol/DiscoveryProtocol.hpp"
#include <spdlog/spdlog.h>

namespace marlin {
namespace beacon {

Beacon::Beacon(const net::SocketAddress &_addr) : addr(_addr), localSocket() {
	localSocket.bind(addr);

	protocol::DiscoveryProtocol<Beacon>::setup(*this);
}

void Beacon::start_listening() {
	localSocket.start_receive(*this);
}

void Beacon::did_receive_packet(
	net::Packet &&p,
	const net::SocketAddress &addr
) {
	protocol::DiscoveryProtocol<Beacon>::did_receive_packet(*this, std::move(p), addr);
}

void Beacon::send(net::Packet &&p, const net::SocketAddress &addr) {
	localSocket.send(std::move(p), addr, *this);
}

void Beacon::did_send_packet(
	net::Packet &&p,
	const net::SocketAddress &addr
) {
	protocol::DiscoveryProtocol<Beacon>::did_send_packet(*this, std::move(p), addr);
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

void Beacon::start_discovery(const net::SocketAddress &addr) {
	protocol::DiscoveryProtocol<Beacon>::send_DISCOVER(*this, addr);
}

} // namespace beacon
} // namespace marlin
