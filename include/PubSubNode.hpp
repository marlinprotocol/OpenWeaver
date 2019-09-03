#ifndef MARLIN_PUBSUB_PUBSUBNODE_HPP
#define MARLIN_PUBSUB_PUBSUBNODE_HPP

#include <marlin/net/udp/UdpTransportFactory.hpp>
#include <marlin/net/tcp/TcpTransportFactory.hpp>
#include <marlin/stream/StreamTransportFactory.hpp>

#include <algorithm>
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
	struct hash<std::tuple<marlin::net::SocketAddress const, uint16_t const>>
	{
		size_t operator()(const std::tuple<marlin::net::SocketAddress const, uint16_t const> &tup) const
		{
			return std::hash<std::string>()(std::get<0>(tup).to_string()) ^
					std::hash<std::uint16_t>()(std::get<1>(tup));
		}
	};
}

namespace marlin {
namespace pubsub {

#define DefaultMaxSubscriptions 5
#define DefaultMsgIDTimerInterval 10000
#define DefaultPeerSelectTimerInterval 60000

struct ReadBuffer {
	std::list<net::Buffer> message_buffer;
	uint64_t message_length = 0;
	uint64_t bytes_remaining = 0;
	std::string channel;
	uint64_t message_id = 0;
};

template<typename PubSubDelegate>
class PubSubNode {

protected:
	typedef stream::StreamTransportFactory<
		PubSubNode<PubSubDelegate>,
		PubSubNode<PubSubDelegate>,
		net::UdpTransportFactory,
		net::UdpTransport
	> BaseTransportFactory;
	typedef stream::StreamTransport<
		PubSubNode<PubSubDelegate>,
		net::UdpTransport
	> BaseTransport;

	typedef std::unordered_set<BaseTransport *> TransportSet;
	typedef std::unordered_map< std::string, TransportSet> TransportSetMap;

	TransportSetMap channel_subscriptions;
	TransportSetMap potential_channel_subscriptions;


	// typedef net::TcpTransportFactory<
	// 	PubSubNode<PubSubDelegate>,
	// 	PubSubNode<PubSubDelegate>
	// > BaseTransportFactory;
	// typedef net::TcpTransport<
	// 	PubSubNode<PubSubDelegate>
	// > BaseTransport;

	BaseTransportFactory f;

	std::unordered_map<std::tuple<net::SocketAddress const, uint16_t const>, ReadBuffer> read_buffers;

	// PubSub
	void did_recv_SUBSCRIBE(BaseTransport &transport, net::Buffer &&bytes);
	void send_SUBSCRIBE(BaseTransport &transport, std::string const channel);

	void did_recv_UNSUBSCRIBE(BaseTransport &transport, net::Buffer &&bytes);
	void send_UNSUBSCRIBE(BaseTransport &transport, std::string const channel);

	void did_recv_RESPONSE(BaseTransport &transport, net::Buffer &&bytes);
	void send_RESPONSE(
		BaseTransport &transport,
		bool success,
		std::string msg_string
	);

	void did_recv_MESSAGE(BaseTransport &transport, net::Buffer &&bytes);
	void send_MESSAGE(
		BaseTransport &transport,
		std::string channel,
		uint64_t message_id,
		const char *data,
		uint64_t size
	);

	virtual void manage_subscribers() {};
	void add_subscriber_to_channel(std::string channel, BaseTransport &transport);

	BaseTransport* find_random_rtt_transport(TransportSet& transport_set);
	BaseTransport* find_min_rtt_transport(TransportSet& transport_set);
	BaseTransport* find_max_rtt_transport(TransportSet& transport_set);
	bool check_tranport_in_set(BaseTransport&, TransportSet& transport_set);

public:
	// Listen delegate
	bool should_accept(net::SocketAddress const &addr);
	void did_create_transport(BaseTransport &transport);

	// Transport delegate
	void did_dial(BaseTransport &transport);
	void did_recv_bytes(BaseTransport &transport, net::Buffer &&bytes);
	void did_send_bytes(BaseTransport &transport, net::Buffer &&bytes);
	void did_close(BaseTransport &transport);

	PubSubNode(const net::SocketAddress &_addr);

	PubSubDelegate *delegate;
	bool should_relay = false;

	int dial(net::SocketAddress const &addr);
	uint64_t send_message_on_channel(
		std::string channel,
		const char *data,
		uint64_t size,
		net::SocketAddress const *excluded = nullptr
	);
	void send_message_on_channel(
		std::string channel,
		uint64_t message_id,
		const char *data,
		uint64_t size,
		net::SocketAddress const *excluded = nullptr
	);

	void subscribe(net::SocketAddress const &addr);
	void unsubscribe(net::SocketAddress const &addr);

	void add_subscriber(net::SocketAddress const &addr);

protected:

	std::uniform_int_distribution<uint64_t> message_id_dist;
	std::mt19937_64 message_id_gen;

	// Message id history for deduplication
	std::vector<std::vector<uint64_t>> message_id_events;
	uint8_t message_id_idx = 0;
	std::unordered_set<uint64_t> message_id_set;

	uv_timer_t message_id_timer;

	static void message_id_timer_cb(uv_timer_t *handle) {
		auto &node = *(PubSubNode<PubSubDelegate> *)handle->data;

		// Overflow behaviour desirable
		node.message_id_idx++;

		for (
			auto iter = node.message_id_events[node.message_id_idx].begin();
			iter != node.message_id_events[node.message_id_idx].end();
			iter = node.message_id_events[node.message_id_idx].erase(iter)
		) {
			node.message_id_set.erase(*iter);
		}
	}

	uv_timer_t peer_selection_timer;

	static void peer_selection_timer_cb(uv_timer_t *handle) {
		auto &node = *(PubSubNode<PubSubDelegate> *)handle->data;

		node.manage_subscribers();
	}
};


// Impl

//---------------- PubSub functions begin ----------------//

template<typename PubSubDelegate>
void PubSubNode<PubSubDelegate>::did_recv_SUBSCRIBE(
	BaseTransport &transport,
	net::Buffer &&bytes
) {
	std::string channel(bytes.data(), bytes.data()+bytes.size());

	add_subscriber_to_channel(channel, transport);

	SPDLOG_DEBUG(
		"Received subscribe on channel {} from {}",
		channel,
		transport.dst_addr.to_string()
	);

	// Send response
	// send_RESPONSE(transport, true, "SUBSCRIBED TO " + channel);
}

template<typename PubSubDelegate>
void PubSubNode<PubSubDelegate>::send_SUBSCRIBE(
	BaseTransport &transport,
	std::string const channel
) {
	char *message = new char[channel.size()+1];

	message[0] = 0;
	std::memcpy(message + 1, channel.data(), channel.size());

	net::Buffer bytes(message, channel.size() + 1);

	SPDLOG_DEBUG(
		"Sending subscribe on channel {} to {}",
		channel,
		transport.dst_addr.to_string()
	);

	transport.send(std::move(bytes));
}

template<typename PubSubDelegate>
void PubSubNode<PubSubDelegate>::did_recv_UNSUBSCRIBE(
	BaseTransport &transport,
	net::Buffer &&bytes
) {
	std::string channel(bytes.data(), bytes.data()+bytes.size());

	channel_subscriptions[channel].erase(&transport);

	SPDLOG_DEBUG(
		"Received unsubscribe on channel {} from {}",
		channel,
		transport.dst_addr.to_string()
	);

	// Send response
	send_RESPONSE(transport, true, "UNSUBSCRIBED FROM " + channel);
}

template<typename PubSubDelegate>
void PubSubNode<PubSubDelegate>::send_UNSUBSCRIBE(
	BaseTransport &transport,
	std::string const channel
) {
	char *message = new char[channel.size()+1];

	message[0] = 1;
	std::memcpy(message + 1, channel.data(), channel.size());

	net::Buffer bytes(message, channel.size() + 1);

	SPDLOG_DEBUG("Sending unsubscribe on channel {} to {}", channel, transport.dst_addr.to_string());

	transport.send(std::move(bytes));
}

template<typename PubSubDelegate>
void PubSubNode<PubSubDelegate>::did_recv_RESPONSE(
	BaseTransport &,
	net::Buffer &&bytes
) {
	bool success __attribute__((unused)) = bytes.data()[0];

	// Hide success
	bytes.cover(1);

	// Process rest of the message
	std::string message(bytes.data(), bytes.data()+bytes.size());

	// Check subscibe/unsubscribe response
	if(message.rfind("UNSUBSCRIBED FROM ", 0) == 0) {
		delegate->did_unsubscribe(*this, message.substr(18));
	} else if(message.rfind("SUBSCRIBED TO ", 0) == 0) {
		delegate->did_subscribe(*this, message.substr(14));
	}

	SPDLOG_DEBUG(
		"Received {} response: {}",
		success == 0 ? "ERROR" : "OK",
		spdlog::to_hex(message.data(), message.data() + message.size())
	);
}

template<typename PubSubDelegate>
void PubSubNode<PubSubDelegate>::send_RESPONSE(
	BaseTransport &transport,
	bool success,
	std::string msg_string
) {
	// 0 for ERROR
	// 1 for OK
	uint64_t tot_msg_size = msg_string.size()+2;
	char *message = new char[tot_msg_size];

	message[0] = 2;
	message[1] = success ? 1 : 0;

	std::memcpy(message + 2, msg_string.data(), msg_string.size());

	net::Buffer m(message, tot_msg_size);

	SPDLOG_DEBUG(
		"Sending {} response: {}",
		success == 0 ? "ERROR" : "OK",
		spdlog::to_hex(message, message + tot_msg_size)
	);
	transport.send(std::move(m));
}

template<typename PubSubDelegate>
void PubSubNode<PubSubDelegate>::did_recv_MESSAGE(
	BaseTransport &transport,
	net::Buffer &&bytes
) {
	uint8_t stream_id = 0;
	auto iter = read_buffers.try_emplace(
		std::make_tuple(transport.dst_addr, stream_id)
	).first;
	auto &read_buffer = iter->second;

	if(read_buffer.bytes_remaining == 0) { // New message
		SPDLOG_DEBUG("New message");

		read_buffer.bytes_remaining = bytes.read_uint64_be(0);
		read_buffer.message_length = read_buffer.bytes_remaining;

		if(read_buffer.message_length > 5000000) {
			read_buffers.erase(std::make_tuple(transport.dst_addr, stream_id));
			transport.close();
			return;
		}

		read_buffer.message_id = bytes.read_uint64_be(8);
		uint16_t channel_length = bytes.read_uint16_be(16);

		// Check overflow
		if((uint16_t)bytes.size() < 18 + channel_length)
			return;

		if(channel_length > 10) {
			read_buffers.erase(std::make_tuple(transport.dst_addr, stream_id));
			transport.close();
			return;
		}

		read_buffer.channel = std::string(bytes.data()+18, bytes.data()+18+channel_length);

		bytes.cover(18 + channel_length);
	}

	// Check if full message has been received
	if(read_buffer.bytes_remaining > bytes.size()) { // Incomplete message
		SPDLOG_DEBUG("Incomplete message");
		read_buffer.bytes_remaining -= bytes.size();
		read_buffer.message_buffer.push_back(std::move(bytes));
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
			bytes.data(),
			read_buffer.bytes_remaining
		);

		// Send it onward
		if(message_id_set.find(read_buffer.message_id) == message_id_set.end()) { // Deduplicate message
			message_id_set.insert(read_buffer.message_id);
			message_id_events[message_id_idx].push_back(read_buffer.message_id);

			if(should_relay) {
				send_message_on_channel(
					read_buffer.channel,
					read_buffer.message_id,
					message.get(),
					read_buffer.message_length,
					&transport.dst_addr
				);
			}

			// Call delegate
			delegate->did_recv_message(
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
void PubSubNode<PubSubDelegate>::send_MESSAGE(
	BaseTransport &transport,
	std::string channel,
	uint64_t message_id,
	const char *data,
	uint64_t size
) {
	char *message = new char[channel.size()+19+size];

	message[0] = 3;

	net::Buffer m(message, channel.size()+19+size);
	m.write_uint64_be(1, size);
	m.write_uint64_be(9, message_id);
	m.write_uint16_be(17, channel.size());
	std::memcpy(message + 19, channel.data(), channel.size());
	std::memcpy(message + 19 + channel.size(), data, size);

	transport.send(std::move(m));
}

//---------------- PubSub functions end ----------------//


//---------------- Listen delegate functions begin ----------------//

template<typename PubSubDelegate>
bool PubSubNode<PubSubDelegate>::should_accept(net::SocketAddress const &) {
	return true;
}

template<typename PubSubDelegate>
void PubSubNode<PubSubDelegate>::did_create_transport(
	BaseTransport &transport
) {
	transport.setup(this);
}

//---------------- Listen delegate functions end ----------------//


//---------------- Transport delegate functions begin ----------------//

template<typename PubSubDelegate>
void PubSubNode<PubSubDelegate>::did_dial(BaseTransport &transport) {
	std::for_each(
		delegate->channels.begin(),
		delegate->channels.end(),
		[&] (std::string const channel) {
			send_SUBSCRIBE(transport, channel);
		}
	);
}

template<typename PubSubDelegate>
void PubSubNode<PubSubDelegate>::did_recv_bytes(
	BaseTransport &transport,
	net::Buffer &&bytes
) {
	// Abort on empty message
	if(bytes.size() == 0)
		return;

	uint8_t stream_id = 0;

	auto iter = read_buffers.try_emplace(
		std::make_tuple(transport.dst_addr, stream_id)
	).first;
	auto &read_buffer = iter->second;

	// Check if it is part of previously incomplete message
	if(read_buffer.bytes_remaining > 0) {
		this->did_recv_MESSAGE(transport, std::move(bytes));
		return;
	}

	uint8_t message_type = bytes.data()[0];

	// Hide message type
	bytes.cover(1);

	switch(message_type) {
		// SUBSCRIBE
		case 0: this->did_recv_SUBSCRIBE(transport, std::move(bytes));
		break;
		// UNSUBSCRIBE
		case 1: this->did_recv_UNSUBSCRIBE(transport, std::move(bytes));
		break;
		// RESPONSE
		case 2: this->did_recv_RESPONSE(transport, std::move(bytes));
		break;
		// MESSAGE
		case 3: this->did_recv_MESSAGE(transport, std::move(bytes));
		break;
	}
}

template<typename PubSubDelegate>
void PubSubNode<PubSubDelegate>::did_send_bytes(
	BaseTransport &,
	net::Buffer &&
) {
}

template<typename PubSubDelegate>
void PubSubNode<PubSubDelegate>::did_close(BaseTransport &transport) {
	std::for_each(
		delegate->channels.begin(),
		delegate->channels.end(),
		[&] (std::string const channel) {
			channel_subscriptions[channel].erase(&transport);
		}
	);
}

//---------------- Transport delegate functions end ----------------//


template<typename PubSubDelegate>
PubSubNode<PubSubDelegate>::PubSubNode(
	const net::SocketAddress &addr
) : message_id_gen(std::random_device()()),
	message_id_events(256)
{
	f.bind(addr);
	f.listen(*this);

	uv_timer_init(uv_default_loop(), &message_id_timer);
	this->message_id_timer.data = (void *)this;
	uv_timer_start(&message_id_timer, &message_id_timer_cb, DefaultMsgIDTimerInterval, DefaultMsgIDTimerInterval);

	uv_timer_init(uv_default_loop(), &peer_selection_timer);
	this->peer_selection_timer.data = (void *)this;
	uv_timer_start(&peer_selection_timer, &peer_selection_timer_cb, DefaultPeerSelectTimerInterval, DefaultPeerSelectTimerInterval);
}

template<typename PubSubDelegate>
int PubSubNode<PubSubDelegate>::dial(net::SocketAddress const &addr) {
	return f.dial(addr, *this);
}



template<typename PubSubDelegate>
uint64_t PubSubNode<PubSubDelegate>::send_message_on_channel(
	std::string channel,
	const char *data,
	uint64_t size,
	net::SocketAddress const *excluded
) {
	uint64_t message_id = this->message_id_dist(this->message_id_gen);
	send_message_on_channel(channel, message_id, data, size, excluded);

	return message_id;
}

template<typename PubSubDelegate>
void PubSubNode<PubSubDelegate>::send_message_on_channel(
	std::string channel,
	uint64_t message_id,
	const char *data,
	uint64_t size,
	net::SocketAddress const *excluded
) {
	auto subscribers = channel_subscriptions[channel];
	for (
		auto it = subscribers.begin();
		it != subscribers.end();
		it++
	) {
		// Exclude given address, usually sender tp prevent loops
		if(excluded != nullptr && (*it)->dst_addr == *excluded)
			continue;
		SPDLOG_DEBUG(
			"Sending message {} on channel {} to {}",
			message_id,
			channel,
			(*it)->dst_addr.to_string()
		);
		send_MESSAGE(**it, channel, message_id, data, size);
	}
}

template<typename PubSubDelegate>
void PubSubNode<PubSubDelegate>::subscribe(net::SocketAddress const &addr) {
	auto *transport = f.get_transport(addr);

	if(transport == nullptr) {
		dial(addr);
		return;
	} else if(!transport->is_active()) {
		return;
	}

	std::for_each(
		delegate->channels.begin(),
		delegate->channels.end(),
		[&] (std::string const channel) {
			send_SUBSCRIBE(*transport, channel);
		}
	);
}

template<typename PubSubDelegate>
void PubSubNode<PubSubDelegate>::unsubscribe(net::SocketAddress const &addr) {
	auto *transport = f.get_transport(addr);

	if(transport == nullptr) {
		return;
	}

	std::for_each(
		delegate->channels.begin(),
		delegate->channels.end(),
		[&] (std::string const channel) {
			send_UNSUBSCRIBE(*transport, channel);
		}
	);
}

template<typename PubSubDelegate>
void PubSubNode<PubSubDelegate>::add_subscriber(net::SocketAddress const &addr) {
	auto *transport = f.get_transport(addr);

	if(transport == nullptr) {
		return;
	}

	std::for_each(
		delegate->channels.begin(),
		delegate->channels.end(),
		[&] (std::string const channel) {
			add_subscriber_to_channel(channel, *transport);
		}
	);
}

// Pick the ones with rtt's -1 too to give them a chance
template<typename PubSubDelegate>
typename PubSubNode<PubSubDelegate>::BaseTransport* PubSubNode<PubSubDelegate>::find_random_rtt_transport(TransportSet& transport_set) {
	
	BaseTransport* to_return = nullptr;
	
	int set_size = transport_set.size();

	if (set_size != 0) {
		int random_to_return = rand() % set_size;
		to_return = *(std::next(transport_set.begin(), random_to_return));
	}

	return to_return;
}

// Pick the ones with rtt's -1 too to give them a chance
template<typename PubSubDelegate>
typename PubSubNode<PubSubDelegate>::BaseTransport* PubSubNode<PubSubDelegate>::find_min_rtt_transport(TransportSet& transport_set) {
	BaseTransport* to_return = nullptr;
	for (auto* temp_transport : transport_set) {
		if (temp_transport->get_rtt() == -1)
			return temp_transport;
		if (to_return == nullptr || temp_transport->get_rtt() < to_return->get_rtt()) {
			to_return = temp_transport;
		}	
	}

	return to_return;
}

template<typename PubSubDelegate>
typename PubSubNode<PubSubDelegate>::BaseTransport* PubSubNode<PubSubDelegate>::find_max_rtt_transport(TransportSet& transport_set) {
	BaseTransport* to_return = nullptr;
	for (auto* temp_transport : transport_set) {
		if (temp_transport->get_rtt() == -1) continue;
		if (to_return == nullptr || temp_transport->get_rtt() > to_return->get_rtt()) {
			to_return = temp_transport;
		}	
	}

	return to_return;
}

template<typename PubSubDelegate>
bool PubSubNode<PubSubDelegate>::check_tranport_in_set(BaseTransport& base_transport, TransportSet& transport_set) {
	if (transport_set.find(&base_transport) == transport_set.end()) {
		return false;
	}

	return true;
}

/* TODO:
1. correct logging
2. send response
*/
template<typename PubSubDelegate>
void PubSubNode<PubSubDelegate>::add_subscriber_to_channel(
	std::string channel,
	BaseTransport &transport) {

	if (channel_subscriptions[channel].size() >= DefaultMaxSubscriptions &&
		!check_tranport_in_set(transport, channel_subscriptions[channel])) {

		SPDLOG_INFO("Adding address: {} to potential subscribers list on channel: {} ",
			transport.dst_addr.to_string(),
			channel);
		potential_channel_subscriptions[channel].insert(&transport);

	}
	else if (!check_tranport_in_set(transport, potential_channel_subscriptions[channel])) {
		SPDLOG_INFO("Adding address: {} to subscribers list on channel: {} ",
			transport.dst_addr.to_string(),
			channel);
		channel_subscriptions[channel].insert(&transport);

		send_RESPONSE(transport, true, "SUBSCRIBED TO " + channel);
	}
}

} // namespace pubsub
} // namespace marlin

#endif // MARLIN_PUBSUB_PUBSUBNODE_HPP
