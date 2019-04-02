#ifndef MARLIN_PUBSUB_PUBSUBNODE_HPP
#define MARLIN_PUBSUB_PUBSUBNODE_HPP

#include <marlin/net/Node.hpp>
#include <marlin/stream/protocol/StreamProtocol.hpp>

namespace marlin {
namespace pubsub {

class PubSubNode: public net::Node<PubSubNode, stream::StreamProtocol> {
public:
	stream::StreamStorage<PubSubNode> stream_storage;

	PubSubNode(const net::SocketAddress &_addr);

	std::list<net::Packet> message_buffer;
	uint64_t message_length = 0;
	uint64_t bytes_remaining = 0;

	void did_receive_bytes(net::Packet &&p, uint16_t stream_id, const net::SocketAddress &addr);

	void did_receive_SUBSCRIBE(net::Packet &&p, uint16_t, const net::SocketAddress &);
	void send_SUBSCRIBE(const net::SocketAddress &addr, std::string channel);

	void did_receive_UNSUBSCRIBE(net::Packet &&p, uint16_t, const net::SocketAddress &);
	void send_UNSUBSCRIBE(const net::SocketAddress &addr, std::string channel);

	void did_receive_RESPONSE(net::Packet &&, uint16_t, const net::SocketAddress &);
	void send_RESPONSE(const net::SocketAddress &);

	void did_receive_MESSAGE(net::Packet &&p, uint16_t, const net::SocketAddress &);
	void send_MESSAGE(const net::SocketAddress &addr, std::string channel, char *data, uint64_t size);
};

} // namespace pubsub
} // namespace marlin

#endif // MARLIN_PUBSUB_PUBSUBNODE_HPP
