#ifndef MARLIN_NET_NODE_HPP
#define MARLIN_NET_NODE_HPP

#include "Socket.hpp"

namespace marlin {
namespace net {

// Base node class implementing CRTP
template<typename NodeType, template<typename> typename ProtocolType>
class Node {
public:
	SocketAddress addr;
	Socket localSocket;

	Node(const SocketAddress &addr);

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
};

// impl

template<typename NodeType, template<typename> typename ProtocolType>
Node<NodeType, ProtocolType>::Node(const SocketAddress &_addr) : addr(_addr), localSocket() {
	localSocket.bind(this->addr);
}

template<typename NodeType, template<typename> typename ProtocolType>
void Node<NodeType, ProtocolType>::start_listening() {
	localSocket.start_receive<NodeType>(static_cast<NodeType &>(*this));
}

template<typename NodeType, template<typename> typename ProtocolType>
void Node<NodeType, ProtocolType>::did_receive_packet(net::Packet &&p, const net::SocketAddress &addr) {
	ProtocolType<NodeType>::did_receive_packet(static_cast<NodeType &>(*this), std::move(p), addr);
}

template<typename NodeType, template<typename> typename ProtocolType>
void Node<NodeType, ProtocolType>::send(net::Packet &&p, const net::SocketAddress &addr) {
	localSocket.send<NodeType>(std::move(p), addr, static_cast<NodeType &>(*this));
}

template<typename NodeType, template<typename> typename ProtocolType>
void Node<NodeType, ProtocolType>::did_send_packet(net::Packet &&p, const net::SocketAddress &addr) {
	ProtocolType<NodeType>::did_send_packet(static_cast<NodeType &>(*this), std::move(p), addr);
}

} // namespace net
} // namespace marlin

#endif // MARLIN_NET_NODE_HPP
