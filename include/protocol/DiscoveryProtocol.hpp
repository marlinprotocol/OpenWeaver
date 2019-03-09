#ifndef MARLIN_BEACON_DISCOVERYPROTOCOL_HPP
#define MARLIN_BEACON_DISCOVERYPROTOCOL_HPP

#include "PingProtocol.hpp"
#include <ctime>
#include <cstring>
#include <spdlog/spdlog.h>

namespace marlin {
namespace protocol {

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
	static std::map<uv_timer_t *, NodeType *> registry;

	static void register_node(uv_timer_t *handle, NodeType *cb) {
		registry[handle] = cb;
	}
	static void deregister_node(uv_timer_t *handle) {
		registry.erase(handle);
	}

	static void timer_cb(uv_timer_t *handle) {
		auto iter = registry.find(handle);
		if (iter == registry.end())
			return;

		auto cur_time = std::time(NULL);

		// Remove stale peers if inactive for a minute
		iter->second->peers.remove_if([cur_time](const auto &p) {
			return std::difftime(cur_time, p.last_receipt_time) > 60;
		});

		// Send heartbeat
		std::for_each(
			iter->second->peers.begin(),
			iter->second->peers.end(),
			[iter](const auto &p) {
				DiscoveryProtocol<NodeType>::send_HEARTBEAT(*iter->second, p.addr);
			}
		);
	}
};

template<typename NodeType>
std::map<uv_timer_t *, NodeType *> DiscoveryProtocol<NodeType>::registry = std::map<uv_timer_t *, NodeType *>();


// Impl

typedef PingPacket DiscoveryPacket;

template<typename NodeType>
void DiscoveryProtocol<NodeType>::setup(
	NodeType &node
) {
	node.peers.clear();

	uv_timer_init(uv_default_loop(), &node.timer);

	DiscoveryProtocol<NodeType>::register_node(&node.timer, &node);

	uv_timer_start(&node.timer, &DiscoveryProtocol<NodeType>::timer_cb, 10000, 10000);
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

	node.add_or_update_receipt_time(addr);
}

template<typename NodeType>
void DiscoveryProtocol<NodeType>::did_send_packet(
	NodeType &node,
	const net::Packet &&p,
	const net::SocketAddress &addr
) {
	auto pp = reinterpret_cast<const DiscoveryPacket *>(&p);
	switch(pp->message()) {
		// DISCOVER
		case 2: spdlog::info("DISCOVER >>> {}", addr.to_string());
		break;
		// PEERLIST
		case 3: spdlog::info("PEERLIST >>> {}", addr.to_string());
		break;
		// HEARTBEAT
		case 4: spdlog::info("HEARTBEAT >>> {}", addr.to_string());
		break;
		default: PingProtocol<NodeType>::did_send_packet(node, std::move(p), addr);
		return;
	}
}

template<typename NodeType>
void DiscoveryProtocol<NodeType>::send_DISCOVER(NodeType &node, const net::SocketAddress &addr) {
	char *message = new char[10] {0, 2};

	net::Packet p(message, 10);
	node.send(std::move(p), addr);
}

template<typename NodeType>
void DiscoveryProtocol<NodeType>::did_receive_DISCOVER(NodeType &node, const net::SocketAddress &addr) {
	spdlog::info("DISCOVER <<< {}", addr.to_string());

	send_PEERLIST(node, addr);
}

template<typename NodeType>
void DiscoveryProtocol<NodeType>::send_PEERLIST(NodeType &node, const net::SocketAddress &addr) {
	char *message = new char[1100] {0, 3};

	size_t size = 3;
	// TODO - Handle overflow
	for(auto iter = node.peers.begin(); iter != node.peers.end() && size < 1100; iter++) {
		auto bytes = iter->addr.serialize();
		std::memcpy(message+size, bytes.data(), 8);
		size += 8;
	}

	net::Packet p(message, size);
	node.send(std::move(p), addr);
}

template<typename NodeType>
void DiscoveryProtocol<NodeType>::did_receive_PEERLIST(NodeType &node, const net::SocketAddress &addr, const net::Packet &&p) {
	spdlog::info("PEERLIST <<< {}", addr.to_string());

	std::vector<unsigned char> bytes(p.data()+3, p.data()+p.size());

	// TODO - Handle overflow
	for(auto iter = bytes.begin(); iter != bytes.end(); iter+=8) {
		net::SocketAddress peer_addr = net::SocketAddress::deserialize(iter);
		spdlog::debug("Peer - {}", peer_addr.to_string());

		if(!(peer_addr == node.addr))
			PingProtocol<NodeType>::send_PING(node, peer_addr);
	}
}

template<typename NodeType>
void DiscoveryProtocol<NodeType>::send_HEARTBEAT(NodeType &node, const net::SocketAddress &addr) {
	char *message = new char[10] {0, 4};

	net::Packet p(message, 10);
	node.send(std::move(p), addr);
}

template<typename NodeType>
void DiscoveryProtocol<NodeType>::did_receive_HEARTBEAT(NodeType &node, const net::SocketAddress &addr) {
	spdlog::info("HEARTBEAT <<< {}", addr.to_string());
}

} // namespace protocol
} // namespace marlin

#endif // MARLIN_BEACON_DISCOVERYPROTOCOL_HPP
