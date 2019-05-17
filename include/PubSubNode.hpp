#ifndef MARLIN_PUBSUB_PUBSUBNODE_HPP
#define MARLIN_PUBSUB_PUBSUBNODE_HPP

#include <marlin/net/Node.hpp>
#include <marlin/stream/protocol/StreamProtocol.hpp>

#include <map>
#include <string>
#include <list>
#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <random>
#include <unordered_set>
#include <uv.h>

namespace std {

	template <>
	struct hash<marlin::net::SocketAddress>
	{
		size_t operator()(const marlin::net::SocketAddress &addr) const
		{
			return std::hash<std::string>()(addr.to_string());
		}
	};
}

namespace marlin {
namespace pubsub {

struct ReadBuffer {
	std::list<net::Packet> message_buffer;
	uint64_t message_length = 0;
	uint64_t bytes_remaining = 0;
	std::string channel;
	uint64_t message_id = 0;
};

typedef std::unordered_set<net::SocketAddress> ListSocketAddress;
typedef std::map<std::string, ListSocketAddress > MapChannelToAddresses;

template<typename PubSubDelegate>
class PubSubNode : public net::Node<PubSubNode<PubSubDelegate>, stream::StreamProtocol> {
public:
	typedef net::Node<PubSubNode<PubSubDelegate>, stream::StreamProtocol> BaseNode;

	stream::StreamStorage<PubSubNode> stream_storage;

	PubSubNode(const net::SocketAddress &_addr);
	~PubSubNode();

	std::map<std::tuple<net::SocketAddress const, uint16_t const>, ReadBuffer *> read_buffers;

	PubSubDelegate *delegate;

	void did_receive_bytes(net::Packet &&p, uint16_t stream_id, const net::SocketAddress &addr);

	void did_receive_SUBSCRIBE(net::Packet &&p, uint16_t, const net::SocketAddress &);
	void send_SUBSCRIBE(const net::SocketAddress &addr, std::string channel);

	void did_receive_UNSUBSCRIBE(net::Packet &&p, uint16_t, const net::SocketAddress &);
	void send_UNSUBSCRIBE(const net::SocketAddress &addr, std::string channel);

	void did_receive_RESPONSE(net::Packet &&, uint16_t, const net::SocketAddress &);
	void send_RESPONSE(bool success, std::string msg_string, const net::SocketAddress &);

	void did_receive_MESSAGE(net::Packet &&p, uint16_t, const net::SocketAddress &);
	void send_MESSAGE(const net::SocketAddress &addr, std::string channel, uint64_t message_id, const char *data, uint64_t size);

	uint64_t send_message_on_channel(std::string channel, const char *data, uint64_t size, net::SocketAddress const *excluded = nullptr);
	void send_message_on_channel(std::string channel, uint64_t message_id, const char *data, uint64_t size, net::SocketAddress const *excluded = nullptr);

private:
	MapChannelToAddresses channel_subscription_map;

	std::uniform_int_distribution<uint64_t> message_id_dist;
	std::mt19937_64 message_id_gen;

	// Message id history for deduplication
	std::vector<std::vector<uint64_t>> message_id_events;
	uint8_t message_id_idx = 0;
	std::unordered_set<uint64_t> message_id_set;

	uv_timer_t message_id_timer;

	static void timer_cb(uv_timer_t *handle) {
		auto &node = *(PubSubNode<PubSubDelegate> *)handle->data;

		// Overflow behaviour desirable
		node.message_id_idx++;

		for(
			auto iter = node.message_id_events[node.message_id_idx].begin();
			iter != node.message_id_events[node.message_id_idx].end();
			iter = node.message_id_events[node.message_id_idx].erase(iter)
		) {
			node.message_id_set.erase(*iter);
		}
	}
};


// Impl

template<typename PubSubDelegate>
PubSubNode<PubSubDelegate>::PubSubNode(
	const net::SocketAddress &_addr
) : BaseNode(_addr),
	message_id_gen(std::random_device()()),
	message_id_events(256)
{
	stream::StreamProtocol<PubSubNode>::setup(*this);

	uv_timer_init(uv_default_loop(), &message_id_timer);
	this->message_id_timer.data = (void *)this;
	uv_timer_start(&message_id_timer, &PubSubNode<PubSubDelegate>::timer_cb, 10000, 10000);
}

template<typename PubSubDelegate>
PubSubNode<PubSubDelegate>::~PubSubNode() {
	// Cleanup allocated ReadBuffers
	for(
		auto iter = this->read_buffers.begin();
		iter != this->read_buffers.end();
		iter++
	) {
		delete iter->second;
	}
}

template<typename PubSubDelegate>
void PubSubNode<PubSubDelegate>::did_receive_bytes(net::Packet &&p, uint16_t stream_id, const net::SocketAddress &addr) {
	// Abort on empty message
	if(p.size() == 0)
		return;

	auto iter = read_buffers.find(std::make_tuple(addr, stream_id));
	if(iter == read_buffers.end()) {
		iter = read_buffers.emplace(
			std::make_tuple(addr, stream_id),
			new ReadBuffer()
		).first;
	}

	auto &read_buffer = *iter->second;

	// Check if it is part of previously incomplete message
	if(read_buffer.bytes_remaining > 0) {
		this->did_receive_MESSAGE(std::move(p), stream_id, addr);
		return;
	}

	uint8_t message_type = p.data()[0];

	// Hide message type
	p.cover(1);

	switch(message_type) {
		// SUBSCRIBE
		case 0: this->did_receive_SUBSCRIBE(std::move(p), stream_id, addr);
		break;
		// UNSUBSCRIBE
		case 1: this->did_receive_UNSUBSCRIBE(std::move(p), stream_id, addr);
		break;
		// RESPONSE
		case 2: this->did_receive_RESPONSE(std::move(p), stream_id, addr);
		break;
		// MESSAGE
		case 3: this->did_receive_MESSAGE(std::move(p), stream_id, addr);
		break;
	}
}

template<typename PubSubDelegate>
void PubSubNode<PubSubDelegate>::did_receive_SUBSCRIBE(net::Packet &&p, uint16_t, const net::SocketAddress &addr) {
	std::string channel(p.data(), p.data()+p.size());

	channel_subscription_map[channel].insert(addr);

	SPDLOG_INFO("Received subscribe on channel {} from {}", channel, addr.to_string());

	// Send response
	send_RESPONSE(true, "SUBSCRIBED TO " + channel, addr);
}

template<typename PubSubDelegate>
void PubSubNode<PubSubDelegate>::send_SUBSCRIBE(const net::SocketAddress &addr, std::string channel) {
	char *message = new char[channel.size()+1];

	message[0] = 0;
	std::memcpy(message + 1, channel.data(), channel.size());

	std::unique_ptr<char[]> p(message);

	SPDLOG_INFO("Sending subscribe on channel {} to {}", channel, addr.to_string());

	stream::StreamProtocol<PubSubNode>::send_data(*this, 0, std::move(p), channel.size() + 1, addr);
}

template<typename PubSubDelegate>
void PubSubNode<PubSubDelegate>::did_receive_UNSUBSCRIBE(net::Packet &&p, uint16_t, const net::SocketAddress &addr) {
	std::string channel(p.data(), p.data()+p.size());

	channel_subscription_map[channel].erase(addr);

	SPDLOG_INFO("Received unsubscribe on channel {} from {}", channel, addr.to_string());

	// Send response
	send_RESPONSE(true, "UNSUBSCRIBED FROM " + channel, addr);
}

template<typename PubSubDelegate>
void PubSubNode<PubSubDelegate>::send_UNSUBSCRIBE(const net::SocketAddress &addr, std::string channel) {
	char *message = new char[channel.size()+1];

	message[0] = 1;
	std::memcpy(message + 1, channel.data(), channel.size());

	std::unique_ptr<char[]> p(message);

	SPDLOG_INFO("Sending unsubscribe on channel {}", channel);

	stream::StreamProtocol<PubSubNode>::send_data(*this, 0, std::move(p), channel.size() + 1, addr);
}

template<typename PubSubDelegate>
void PubSubNode<PubSubDelegate>::did_receive_RESPONSE(net::Packet &&p, uint16_t, const net::SocketAddress &) {
	bool success = p.data()[0];

	// Hide success
	p.cover(1);

	// Process rest of the message
	std::string message(p.data(), p.data()+p.size());

	// Check subscibe/unsubscribe response
	if(message.rfind("UNSUBSCRIBED FROM ", 0) == 0) {
		delegate->did_unsubscribe(*this, message.substr(18));
	} else if(message.rfind("SUBSCRIBED TO ", 0) == 0) {
		delegate->did_subscribe(*this, message.substr(14));
	}

	SPDLOG_INFO("Received {} response: {}", success == 0 ? "ERROR" : "OK", spdlog::to_hex(message.data(), message.data()+message.size()));
}

template<typename PubSubDelegate>
void PubSubNode<PubSubDelegate>::send_RESPONSE(bool success, std::string msg_string, const net::SocketAddress &addr) {
	// 0 for ERROR
	// 1 for OK
	uint64_t tot_msg_size = msg_string.size()+2;
	char *message = new char[tot_msg_size];

	message[0] = 2;
	message[1] = success ? 1 : 0;

	std::memcpy(message + 2, msg_string.data(), msg_string.size());

	std::unique_ptr<char[]> p(message);

	SPDLOG_INFO("Sending {} response: {}", success == 0 ? "ERROR" : "OK", spdlog::to_hex(message, message + tot_msg_size));
	stream::StreamProtocol<PubSubNode>::send_data(*this, 0, std::move(p), tot_msg_size, addr);
}

template<typename PubSubDelegate>
void PubSubNode<PubSubDelegate>::did_receive_MESSAGE(net::Packet &&p, uint16_t stream_id, const net::SocketAddress &addr) {
	auto iter = read_buffers.find(std::make_tuple(addr, stream_id));
	if(iter == read_buffers.end()) {
		iter = read_buffers.emplace(
			std::make_tuple(addr, stream_id),
			new ReadBuffer()
		).first;
	}

	auto &read_buffer = *iter->second;

	if(read_buffer.bytes_remaining == 0) { // New message
		SPDLOG_DEBUG("New message");
		// Check overflow
		if(p.size() < 8)
			return;

		uint64_t n_length;
		std::memcpy(&n_length, p.data(), 8);
		read_buffer.bytes_remaining = ntohll(n_length);
		read_buffer.message_length = read_buffer.bytes_remaining;

		// Check overflow
		if(p.size() < 16)
			return;

		uint64_t n_message_id;
		std::memcpy(&n_message_id, p.data()+8, 8);
		read_buffer.message_id = ntohll(n_message_id);

		// Check overflow
		if(p.size() < 18)
			return;

		uint16_t n_channel_length;
		std::memcpy(&n_channel_length, p.data()+16, 2);
		uint16_t channel_length = ntohs(n_channel_length);

		// Check overflow
		if((uint16_t)p.size() < 18 + channel_length)
			return;

		read_buffer.channel = std::string(p.data()+18, p.data()+18+channel_length);

		p.cover(18 + channel_length);
	}

	// Check if full message has been received
	if(read_buffer.bytes_remaining > p.size()) { // Incomplete message
		SPDLOG_DEBUG("Incomplete message");
		read_buffer.message_buffer.push_back(std::move(p));
		read_buffer.bytes_remaining -= p.size();
	} else { // Full message
		SPDLOG_DEBUG("Full message");
		// Assemble final message
		std::unique_ptr<char[]> message(new char[read_buffer.message_length]);
		uint64_t offset = 0;
		for(
			auto iter = read_buffer.message_buffer.begin();
			iter != read_buffer.message_buffer.end();
			iter = read_buffer.message_buffer.erase(iter)
		) {
			std::memcpy(message.get() + offset, iter->data(), iter->size());
			offset += iter->size();
		}
		// Read only bytes_remaining bytes from final packet to prevent buffer overrun
		std::memcpy(
			message.get() + offset,
			p.data(),
			read_buffer.bytes_remaining
		);

		// Send it onward
		if(message_id_set.find(read_buffer.message_id) == message_id_set.end()) { // Deduplicate message
			message_id_set.insert(read_buffer.message_id);
			message_id_events[message_id_idx].push_back(read_buffer.message_id);
			send_message_on_channel(
				read_buffer.channel,
				read_buffer.message_id,
				message.get(),
				read_buffer.message_length,
				&addr
			);

			// Call delegate
			delegate->did_receive_message(
				*this,
				std::move(message),
				read_buffer.message_length,
				read_buffer.channel,
				read_buffer.message_id
			);
		}

		// Reset state. Message buffer should already be empty.
		read_buffer.bytes_remaining = 0;
		read_buffer.message_length = 0;
		read_buffer.channel.clear();
	}
}

template<typename PubSubDelegate>
void PubSubNode<PubSubDelegate>::send_MESSAGE(const net::SocketAddress &addr, std::string channel, uint64_t message_id, const char *data, uint64_t size) {
	char *message = new char[channel.size()+19+size];

	message[0] = 3;

	uint64_t n_size = htonll(size);
	std::memcpy(message + 1, &n_size, 8);

	uint64_t n_message_id = htonll(message_id);
	std::memcpy(message + 9, &n_message_id, 8);

	uint16_t n_channel_length = htons(channel.size());
	std::memcpy(message + 17, &n_channel_length, 2);
	std::memcpy(message + 19, channel.data(), channel.size());

	std::memcpy(message + 19 + channel.size(), data, size);

	std::unique_ptr<char[]> p(message);
	stream::StreamProtocol<PubSubNode>::send_data(*this, 0, std::move(p), channel.size()+19+size, addr);
}

template<typename PubSubDelegate>
uint64_t PubSubNode<PubSubDelegate>::send_message_on_channel(std::string channel, const char *data, uint64_t size, net::SocketAddress const *excluded) {
	uint64_t message_id = this->message_id_dist(this->message_id_gen);
	send_message_on_channel(channel, message_id, data, size, excluded);

	return message_id;
}

template<typename PubSubDelegate>
void PubSubNode<PubSubDelegate>::send_message_on_channel(std::string channel, uint64_t message_id, const char *data, uint64_t size, net::SocketAddress const *excluded) {
	for (
		ListSocketAddress::iterator it = channel_subscription_map[channel].begin();
		it != channel_subscription_map[channel].end();
		it++
	) {
		// Exclude given address, usually sender tp prevent loops
		if(excluded != nullptr && *it == *excluded)
			continue;
		SPDLOG_DEBUG("Sending message {} on channel {} to {}", message_id, channel, (*it).to_string());
		send_MESSAGE(*it, channel, message_id, data, size);
	}
}

} // namespace pubsub
} // namespace marlin

#endif // MARLIN_PUBSUB_PUBSUBNODE_HPP
