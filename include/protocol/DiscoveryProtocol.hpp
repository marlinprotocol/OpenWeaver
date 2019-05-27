#ifndef MARLIN_BEACON_DISCOVERYPROTOCOL_HPP
#define MARLIN_BEACON_DISCOVERYPROTOCOL_HPP

#include "PingProtocol.hpp"
#include "Peer.hpp"
#include <ctime>
#include <cstring>
#include <spdlog/spdlog.h>
#include <list>

namespace marlin {
namespace beacon {

template<typename Delegate>
struct DiscoveryStorage {
	Delegate *delegate;

	std::list<Peer> peers;
	net::SocketAddress beacon_addr;

	uv_timer_t timer;
};


template<typename NodeType>
class DiscoveryProtocol {
public:
	static void setup(NodeType &node);

	static void did_receive_packet(
		NodeType &node,
		const net::Packet &&p,
		const net::SocketAddress &addr
	);

	static void did_send_packet(
		NodeType &node,
		const net::Packet &&p,
		const net::SocketAddress &addr
	);

	static void did_receive_DISCOVER(NodeType &node, const net::SocketAddress &addr);
	static void send_DISCOVER(NodeType &node, const net::SocketAddress &addr);

	static void did_receive_PEERLIST(NodeType &node, const net::SocketAddress &addr, const net::Packet &&p);
	static void send_PEERLIST(NodeType &node, const net::SocketAddress &addr);

	static void did_receive_HEARTBEAT(NodeType &node, const net::SocketAddress &addr);
	static void send_HEARTBEAT(NodeType &node, const net::SocketAddress &addr);

protected:
	static void timer_cb(uv_timer_t *handle);
};

// Impl

typedef PingPacket DiscoveryPacket;

template<typename NodeType>
void DiscoveryProtocol<NodeType>::setup(
	NodeType &node
) {
	node.protocol_storage.peers.clear();

	uv_timer_init(uv_default_loop(), &node.protocol_storage.timer);
	// Store node for callback later
	node.protocol_storage.timer.data = (void *)&node;

	uv_timer_start(
		&node.protocol_storage.timer,
		&DiscoveryProtocol<NodeType>::timer_cb,
		10000,
		10000
	);
}


template<typename NodeType>
void DiscoveryProtocol<NodeType>::did_receive_packet(
	NodeType &node,
	const net::Packet &&p,
	const net::SocketAddress &addr
) {
	auto pp = reinterpret_cast<const DiscoveryPacket *>(&p);
	switch(pp->message()) {
		// DISCOVER
		case 2: did_receive_DISCOVER(node, addr);
		break;
		// PEERLIST
		case 3: did_receive_PEERLIST(node, addr, std::move(p));
		break;
		// HEARTBEAT
		case 4: did_receive_HEARTBEAT(node, addr);
		break;
		default: PingProtocol<NodeType>::did_receive_packet(node, std::move(p), addr);
		break;
	}

	// Update receipt time
	std::list<Peer>::iterator iter = std::find(
		node.protocol_storage.peers.begin(),
		node.protocol_storage.peers.end(),
		addr
	);
	if(iter == node.protocol_storage.peers.end()) {
		iter = node.protocol_storage.peers.insert(iter, Peer(addr, std::time(NULL)));
		SPDLOG_DEBUG("New peer: {}", addr.to_string());

		node.protocol_storage.delegate->handle_new_peer(addr);
	} else {
		iter->last_receipt_time = std::time(NULL);
		SPDLOG_DEBUG("Old peer: {}", addr.to_string());
	}
}

template<typename NodeType>
void DiscoveryProtocol<NodeType>::did_send_packet(
	NodeType &node,
	const net::Packet &&p,
	const net::SocketAddress &addr __attribute__((unused))
) {
	auto pp = reinterpret_cast<const DiscoveryPacket *>(&p);
	switch(pp->message()) {
		// DISCOVER
		case 2: SPDLOG_DEBUG("DISCOVER >>> {}", addr.to_string());
		break;
		// PEERLIST
		case 3: SPDLOG_DEBUG("PEERLIST >>> {}", addr.to_string());
		break;
		// HEARTBEAT
		case 4: SPDLOG_DEBUG("HEARTBEAT >>> {}", addr.to_string());
		break;
		default: PingProtocol<NodeType>::did_send_packet(node, std::move(p), addr);
		return;
	}
}

template<typename NodeType>
void DiscoveryProtocol<NodeType>::send_DISCOVER(
	NodeType &node,
	const net::SocketAddress &addr
) {
	char *message = new char[10] {0, 2};

	net::Packet p(message, 10);
	node.send(std::move(p), addr);
}

template<typename NodeType>
void DiscoveryProtocol<NodeType>::did_receive_DISCOVER(
	NodeType &node,
	const net::SocketAddress &addr
) {
	SPDLOG_DEBUG("DISCOVER <<< {}", addr.to_string());

	send_PEERLIST(node, addr);
}

template<typename NodeType>
void DiscoveryProtocol<NodeType>::send_PEERLIST(
	NodeType &node,
	const net::SocketAddress &addr
) {
	char *message = new char[1100] {0, 3};

	size_t size = 2;

	// TODO - Handle overflow
	for(
		auto iter = node.protocol_storage.peers.begin();
		iter != node.protocol_storage.peers.end() && size < 1100;
		iter++
	) {
		if(iter->addr == addr) continue;

		auto bytes = iter->addr.serialize();
		std::memcpy(message+size, bytes.data(), 8);
		size += 8;
	}

	net::Packet p(message, size);
	node.send(std::move(p), addr);
}

template<typename NodeType>
void DiscoveryProtocol<NodeType>::did_receive_PEERLIST(
	NodeType &node,
	const net::SocketAddress &addr __attribute__((unused)),
	const net::Packet &&p
) {
	SPDLOG_DEBUG("PEERLIST <<< {}", addr.to_string());

	std::vector<unsigned char> bytes(p.data()+2, p.data()+p.size());

	// TODO - Handle overflow
	for(auto iter = bytes.begin(); iter != bytes.end(); iter+=8) {
		net::SocketAddress peer_addr = net::SocketAddress::deserialize(iter);
		SPDLOG_DEBUG("Peer - {}", peer_addr.to_string());

		if(!(peer_addr == node.addr))
			PingProtocol<NodeType>::send_PING(node, peer_addr);
	}
}

template<typename NodeType>
void DiscoveryProtocol<NodeType>::send_HEARTBEAT(
	NodeType &node,
	const net::SocketAddress &addr
) {
	char *message = new char[10] {0, 4};

	net::Packet p(message, 10);
	node.send(std::move(p), addr);
}

template<typename NodeType>
void DiscoveryProtocol<NodeType>::did_receive_HEARTBEAT(
	NodeType &,
	const net::SocketAddress &addr __attribute__((unused))
) {
	SPDLOG_DEBUG("HEARTBEAT <<< {}", addr.to_string());
}

template<typename NodeType>
void DiscoveryProtocol<NodeType>::timer_cb(uv_timer_t *handle) {
	auto node = (NodeType *)handle->data;

	auto cur_time = std::time(NULL);

	// Remove stale peers if inactive for a minute
	node->protocol_storage.peers.remove_if([cur_time](const Peer &p) {
		return std::difftime(cur_time, p.last_receipt_time) > 60;
	});

	// Send heartbeat
	std::for_each(
		node->protocol_storage.peers.begin(),
		node->protocol_storage.peers.end(),
		[node](const Peer &p) {
			DiscoveryProtocol<NodeType>::send_HEARTBEAT(*node, p.addr);
		}
	);

	// Discover any new nodes
	if(node->protocol_storage.beacon_addr == net::SocketAddress())
		return;
	DiscoveryProtocol<NodeType>::send_DISCOVER(
		*node,
		node->protocol_storage.beacon_addr
	);
}

} // namespace beacon
} // namespace marlin

#endif // MARLIN_BEACON_DISCOVERYPROTOCOL_HPP
