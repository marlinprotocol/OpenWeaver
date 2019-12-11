/*! \file PubSubClient.hpp
    \brief Containing provisions for Publish Subscribe (PubSub) client  functionality
*/

#ifndef MARLIN_PUBSUB_PUBSUBCLIENT_HPP
#define MARLIN_PUBSUB_PUBSUBCLIENT_HPP

#include <marlin/net/udp/UdpTransportFactory.hpp>
#include <marlin/net/tcp/TcpTransportFactory.hpp>
#include <marlin/stream/StreamTransportFactory.hpp>
#include <marlin/lpf/LpfTransportFactory.hpp>

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

#include <marlin/pubsub/PubSubTransportSet.hpp>


namespace marlin {
namespace pubsub {

#define DefaultMaxSubscriptions 2
#define DefaultMsgIDTimerInterval 10000
#define DefaultPeerSelectTimerInterval 60000

//! Class containing the Pub-Sub Client functionality
/*!
	Uses the custom marlin-StreamTransport for message delivery

	Important functions:
	\li subscribe(publisher_address)
	\li unsubsribe(publisher_address)
	\li send_message_on_channel(channel, message)
*/
template<typename PubSubDelegate>
class PubSubClient {
public:
	template<typename ListenDelegate, typename TransportDelegate>
	using UdpStreamTransportFactory = stream::StreamTransportFactory<
		ListenDelegate,
		TransportDelegate,
		net::UdpTransportFactory,
		net::UdpTransport
	>;

	template<typename Delegate>
	using UdpStreamTransport = stream::StreamTransport<
		Delegate,
		net::UdpTransport
	>;

	using BaseTransportFactory = lpf::LpfTransportFactory<
		PubSubClient<PubSubDelegate>,
		PubSubClient<PubSubDelegate>,
		UdpStreamTransportFactory,
		UdpStreamTransport
	>;
	using BaseTransport = lpf::LpfTransport<
		PubSubClient<PubSubDelegate>,
		UdpStreamTransport
	>;

	using TransportSet = PubSubTransportSet<BaseTransport>;
	using TransportSetMap = std::unordered_map<std::string, TransportSet>;

	TransportSetMap channel_subscriptions;
	TransportSetMap potential_channel_subscriptions;

	int get_num_active_subscribers(std::string channel);
	void add_subscriber_to_channel(std::string channel, BaseTransport &transport);
	void add_subscriber_to_potential_channel(std::string channel, BaseTransport &transport);
	void remove_subscriber_from_channel(std::string channel, BaseTransport &transport);
	void remove_subscriber_from_potential_channel(std::string channel, BaseTransport &transport);

private:
	BaseTransportFactory f;

	// PubSub
	void did_recv_SUBSCRIBE(BaseTransport &transport, net::Buffer &&message);
	void send_SUBSCRIBE(BaseTransport &transport, std::string const channel);

	void did_recv_UNSUBSCRIBE(BaseTransport &transport, net::Buffer &&message);
	void send_UNSUBSCRIBE(BaseTransport &transport, std::string const channel);

	void did_recv_RESPONSE(BaseTransport &transport, net::Buffer &&message);
	void send_RESPONSE(
		BaseTransport &transport,
		bool success,
		std::string msg_string
	);

	void did_recv_MESSAGE(BaseTransport &transport, net::Buffer &&message);
	void send_MESSAGE(
		BaseTransport &transport,
		std::string channel,
		uint64_t message_id,
		const char *data,
		uint64_t size
	);

	void send_HEARTBEAT(BaseTransport &transport);

public:
	// Listen delegate
	bool should_accept(net::SocketAddress const &addr);
	void did_create_transport(BaseTransport &transport);

	// Transport delegate
	void did_dial(BaseTransport &transport);
	void did_recv_message(BaseTransport &transport, net::Buffer &&message);
	void did_send_message(BaseTransport &transport, net::Buffer &&message);
	void did_close(BaseTransport &transport);

	PubSubClient(const net::SocketAddress &_addr);

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

private:
	std::uniform_int_distribution<uint64_t> message_id_dist;
	std::mt19937_64 message_id_gen;

	// Message id history for deduplication
	std::vector<std::vector<uint64_t>> message_id_events;
	uint8_t message_id_idx = 0;
	std::unordered_set<uint64_t> message_id_set;

	uv_timer_t message_id_timer;

	static void message_id_timer_cb(uv_timer_t *handle) {
		auto &node = *(PubSubClient<PubSubDelegate> *)handle->data;

		// Overflow behaviour desirable
		node.message_id_idx++;

		for (
			auto iter = node.message_id_events[node.message_id_idx].begin();
			iter != node.message_id_events[node.message_id_idx].end();
			iter = node.message_id_events[node.message_id_idx].erase(iter)
		) {
			node.message_id_set.erase(*iter);
		}

		std::for_each(
			node.delegate->channels.begin(),
			node.delegate->channels.end(),
			[&] (std::string const channel) {
				for (auto* transport : node.channel_subscriptions[channel]) {
					node.send_HEARTBEAT(*transport);
				}
				for (auto* pot_transport : node.potential_channel_subscriptions[channel]) {
					node.send_HEARTBEAT(*pot_transport);
				}
			}
		);
	}

	uv_timer_t peer_selection_timer;

	static void peer_selection_timer_cb(uv_timer_t *handle) {
		auto &node = *(PubSubClient<PubSubDelegate> *)handle->data;

		std::for_each(
			node.delegate->channels.begin(),
			node.delegate->channels.end(),
			[&] (std::string const channel) {
				node.delegate->manage_subscribers(channel, node.channel_subscriptions[channel], node.potential_channel_subscriptions[channel]);
			}
		);
	}

public:
	void cut_through_recv_start(BaseTransport &, uint16_t, uint64_t) {}
	void cut_through_recv_bytes(BaseTransport &, uint16_t, net::Buffer &&) {}
	void cut_through_recv_end(BaseTransport &, uint16_t) {}
	void cut_through_recv_reset(BaseTransport &, uint16_t) {}
};


// Impl

//---------------- PubSub functions begin ----------------//

//! Callback on receipt of subscribe request
/*!
	\param transport StreamTransport instance to be added to subsriber list
	\param bytes Buffer containing the channel name to add the subscriber to
*/
template<typename PubSubDelegate>
void PubSubClient<PubSubDelegate>::did_recv_SUBSCRIBE(
	BaseTransport &transport,
	net::Buffer &&bytes
) {
	std::string channel(bytes.data(), bytes.data()+bytes.size());

	SPDLOG_DEBUG(
		"Received subscribe on channel {} from {}",
		channel,
		transport.dst_addr.to_string()
	);

	add_subscriber_to_channel(channel, transport);
}


/*!
	\verbatim

	SUBSCRIBE (0x00)
	Channel as payload.

	Message format:

	 0               1               2               3
	 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
	+++++++++++++++++++++++++++++++++
	|      0x00     |      0x00     |
	-----------------------------------------------------------------
	|                         Channel Name                        ...
	+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	\endverbatim
*/
template<typename PubSubDelegate>
void PubSubClient<PubSubDelegate>::send_SUBSCRIBE(
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

//! Callback on receipt of unsubscribe request
/*!
	\param transport StreamTransport instance to be removed from subsriber list
	\param bytes Buffer containing the channel name to remove the subscriber from
*/
template<typename PubSubDelegate>
void PubSubClient<PubSubDelegate>::did_recv_UNSUBSCRIBE(
	BaseTransport &transport,
	net::Buffer &&bytes
) {
	std::string channel(bytes.data(), bytes.data()+bytes.size());

	SPDLOG_DEBUG(
		"Received unsubscribe on channel {} from {}",
		channel,
		transport.dst_addr.to_string()
	);

	remove_subscriber_from_channel(channel, transport);
}


/*!
	\verbatim

	UNSUBSCRIBE (0x01)
	Channel as payload.

	Message format:

	 0               1               2               3
	 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
	+++++++++++++++++++++++++++++++++
	|      0x00     |      0x01     |
	-----------------------------------------------------------------
	|                         Channel Name                        ...
	+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	\endverbatim
*/
template<typename PubSubDelegate>
void PubSubClient<PubSubDelegate>::send_UNSUBSCRIBE(
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

/*!
    Callback on receipt of response
*/
template<typename PubSubDelegate>
void PubSubClient<PubSubDelegate>::did_recv_RESPONSE(
	BaseTransport &,
	net::Buffer &&bytes
) {
	bool success __attribute__((unused)) = bytes.data()[0];

	// Hide success
	bytes.cover(1);

	// Process rest of the message
	std::string message(bytes.data(), bytes.data()+bytes.size());

	// Check subscribe/unsubscribe response
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


/*!
	\verbatim

	RESPONSE (0x02)

	Payload contains a byte representing the reponse type(currently a OK/ERROR flag) followed by an arbitrary response message.

	Format:

	 0               1               2               3
	 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
	+++++++++++++++++++++++++++++++++++++++++++++++++
	|      0x00     |      0x02     |      Type     |
	-----------------------------------------------------------------
	|                            Message                          ...
	+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	\endverbatim
*/
template<typename PubSubDelegate>
void PubSubClient<PubSubDelegate>::send_RESPONSE(
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

//! Callback on receipt of message data
/*!
	\li reassembles the fragmented packets received by the streamTransport back into meaninfull data component
	\li relay/forwards the message to other subscriptions on the channel if the should_relay flag is true
	\li performs message deduplication i.e doesnt relay the message if it has already been relayed recently
*/
template<typename PubSubDelegate>
void PubSubClient<PubSubDelegate>::did_recv_MESSAGE(
	BaseTransport &transport,
	net::Buffer &&bytes
) {
	auto message_id = bytes.read_uint64_be(0);
	auto channel_length = bytes.read_uint16_be(8);

	// Check overflow
	if((uint16_t)bytes.size() < 10 + channel_length)
		return;

	if(channel_length > 10) {
		SPDLOG_ERROR("Channel too long: {}", channel_length);
		transport.close();
		return;
	}

	auto channel = std::string(bytes.data()+10, bytes.data()+10+channel_length);

	bytes.cover(10 + channel_length);

	// Send it onward
	if(message_id_set.find(message_id) == message_id_set.end()) { // Deduplicate message
		message_id_set.insert(message_id);
		message_id_events[message_id_idx].push_back(message_id);

		if(should_relay) {
			send_message_on_channel(
				channel,
				message_id,
				bytes.data(),
				bytes.size(),
				&transport.dst_addr
			);
		}

		// Call delegate
		delegate->did_recv_message(
			*this,
			std::move(bytes),
			channel,
			message_id
		);
	}
}

/*!
	\verbatim

	MESSAGE (0x03)

	Payload contains a 8 byte message length, a 8 byte message id, channel details and the message data.

	FORMAT:

	 0               1               2               3
	 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
	+++++++++++++++++++++++++++++++++
	|      0x00     |      0x03     |
	-----------------------------------------------------------------
	|                                                               |
	----                      Message Length                     ----
	|                                                               |
	-----------------------------------------------------------------
	|                                                               |
	----                        Message ID                       ----
	|                                                               |
	-----------------------------------------------------------------
	|      Channel name length      |
	-----------------------------------------------------------------
	|                         Channel Name                        ...
	-----------------------------------------------------------------
	|                         Message Data                        ...
	+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	\endverbatim
*/
template<typename PubSubDelegate>
void PubSubClient<PubSubDelegate>::send_MESSAGE(
	BaseTransport &transport,
	std::string channel,
	uint64_t message_id,
	const char *data,
	uint64_t size
) {
	char *message = new char[channel.size()+11+size];

	message[0] = 3;

	net::Buffer m(message, channel.size()+11+size);
	m.write_uint64_be(1, message_id);
	m.write_uint16_be(9, channel.size());
	std::memcpy(message + 11, channel.data(), channel.size());
	std::memcpy(message + 11 + channel.size(), data, size);

	transport.send(std::move(m));
}

template<typename PubSubDelegate>
void PubSubClient<PubSubDelegate>::send_HEARTBEAT(
	BaseTransport &transport
) {
	char *message = new char[1];

	message[0] = 4;

	net::Buffer m(message, 1);

	transport.send(std::move(m));
}

//---------------- PubSub functions end ----------------//


//---------------- Listen delegate functions begin ----------------//

template<typename PubSubDelegate>
bool PubSubClient<PubSubDelegate>::should_accept(net::SocketAddress const &) {
	return true;
}

template<typename PubSubDelegate>
void PubSubClient<PubSubDelegate>::did_create_transport(
	BaseTransport &transport
) {
	transport.setup(this);
}

//---------------- Listen delegate functions end ----------------//


//---------------- Transport delegate functions begin ----------------//

template<typename PubSubDelegate>
void PubSubClient<PubSubDelegate>::did_dial(BaseTransport &transport) {
	std::for_each(
		delegate->channels.begin(),
		delegate->channels.end(),
		[&] (std::string const channel) {
			add_subscriber_to_channel(channel, transport);
		}
	);
}

//! Receives the bytes/packet fragments from StreamTransport and processes them
/*!
	Determines the type of packet by reading the first byte and redirects the packet to appropriate function for further processing

	\verbatim

	first-byte	:	type
	0			:	subscribe
	1			:	unsubscribe
	2			:	response
	3			:	message

	\endverbatim
*/
template<typename PubSubDelegate>
void PubSubClient<PubSubDelegate>::did_recv_message(
	BaseTransport &transport,
	net::Buffer &&bytes
) {
	// Abort on empty message
	if(bytes.size() == 0)
		return;

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
		// HEARTBEAT
		case 4:
		break;
	}
}

template<typename PubSubDelegate>
void PubSubClient<PubSubDelegate>::did_send_message(
	BaseTransport &,
	net::Buffer &&
) {
}

template<typename PubSubDelegate>
void PubSubClient<PubSubDelegate>::did_close(BaseTransport &transport) {
	std::for_each(
		delegate->channels.begin(),
		delegate->channels.end(),
		[&] (std::string const channel) {
			channel_subscriptions[channel].erase(&transport);
			potential_channel_subscriptions[channel].erase(&transport);
		}
	);
}

//---------------- Transport delegate functions end ----------------//


template<typename PubSubDelegate>
PubSubClient<PubSubDelegate>::PubSubClient(
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
int PubSubClient<PubSubDelegate>::dial(net::SocketAddress const &addr) {
	return f.dial(addr, *this);
}


//! sends messages over a given channel
/*!
	\param channel name of channel to send message on
	\param data char* byte sequence of message to send
	\param size of the message to send
	\param excluded avoid this address while sending the message
*/
template<typename PubSubDelegate>
uint64_t PubSubClient<PubSubDelegate>::send_message_on_channel(
	std::string channel,
	const char *data,
	uint64_t size,
	net::SocketAddress const *excluded
) {
	uint64_t message_id = this->message_id_dist(this->message_id_gen);
	send_message_on_channel(channel, message_id, data, size, excluded);

	return message_id;
}

//! sends messages over a given channel
/*!
	\param channel name of channel to send message on
	\param message_id msg id
	\param data char* byte sequence of message to send
	\param size of the message to send
	\param excluded avoid this address while sending the message
*/
template<typename PubSubDelegate>
void PubSubClient<PubSubDelegate>::send_message_on_channel(
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
		if(size > 50000) {
			char *message = new char[channel.size()+11+size];

			message[0] = 3;

			net::Buffer m(message, channel.size()+11+size);
			m.write_uint64_be(1, message_id);
			m.write_uint16_be(9, channel.size());
			std::memcpy(message + 11, channel.data(), channel.size());
			std::memcpy(message + 11 + channel.size(), data, size);

			auto res = (*it)->cut_through_send(std::move(m));

			// TODO: Handle better
			if(res < 0) {
				SPDLOG_ERROR("Cut through send failed");
				(*it)->close();
			}
		} else {
			send_MESSAGE(**it, channel, message_id, data, size);
		}
	}
}

//! subscribes to given publisher
/*!
	\param addr publisher address to subscribe to
*/
template<typename PubSubDelegate>
void PubSubClient<PubSubDelegate>::subscribe(net::SocketAddress const &addr) {
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
			add_subscriber_to_channel(channel, *transport);
		}
	);
}

//! unsubscribes from given publisher
/*!
	\param addr publisher address to unsubscribe from
*/
template<typename PubSubDelegate>
void PubSubClient<PubSubDelegate>::unsubscribe(net::SocketAddress const &addr) {
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

//! adds given address as subscriber to all the channels specified by delegate
/*!
	\param addr address to add to the subscriber lists
*/
template<typename PubSubDelegate>
void PubSubClient<PubSubDelegate>::add_subscriber(net::SocketAddress const &addr) {
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

template<typename PubSubDelegate>
int PubSubClient<PubSubDelegate>::get_num_active_subscribers(
	std::string channel) {
	return channel_subscriptions[channel].size();
}

template<typename PubSubDelegate>
void PubSubClient<PubSubDelegate>::add_subscriber_to_channel(
	std::string channel,
	BaseTransport &transport
) {
	if (!channel_subscriptions[channel].check_tranport_in_set(transport)) {
		if (channel_subscriptions[channel].size() >= DefaultMaxSubscriptions) {
			add_subscriber_to_potential_channel(channel, transport);
			return;
		}
		SPDLOG_INFO("Adding address: {} to subscribers list on channel: {} ",
			transport.dst_addr.to_string(),
			channel);
		channel_subscriptions[channel].insert(&transport);
		// send_RESPONSE(transport, true, "SUBSCRIBED TO " + channel);
	} else {
		send_SUBSCRIBE(transport, channel);
	}
}

template<typename PubSubDelegate>
void PubSubClient<PubSubDelegate>::add_subscriber_to_potential_channel(
	std::string channel,
	BaseTransport &transport) {

	if (!potential_channel_subscriptions[channel].check_tranport_in_set(transport)) {
		SPDLOG_INFO("Adding address: {} to potential subscribers list on channel: {} ",
			transport.dst_addr.to_string(),
			channel);
		potential_channel_subscriptions[channel].insert(&transport);
	}
}

template<typename PubSubDelegate>
void PubSubClient<PubSubDelegate>::remove_subscriber_from_channel(
	std::string channel,
	BaseTransport &transport) {

	if (channel_subscriptions[channel].check_tranport_in_set(transport)) {
		SPDLOG_INFO("Removing address: {} from subscribers list on channel: {} ",
			transport.dst_addr.to_string(),
			channel);
		send_UNSUBSCRIBE(transport, channel);
		channel_subscriptions[channel].erase(&transport);

		// Send response
		// send_RESPONSE(transport, true, "UNSUBSCRIBED FROM " + channel);
	}
}

template<typename PubSubDelegate>
void PubSubClient<PubSubDelegate>::remove_subscriber_from_potential_channel(
	std::string channel,
	BaseTransport &transport) {

	if (potential_channel_subscriptions[channel].check_tranport_in_set(transport)) {
		SPDLOG_INFO("Removing address: {} from potential subscribers list on channel: {} ",
			transport.dst_addr.to_string(),
			channel);
		potential_channel_subscriptions[channel].erase(&transport);
	}
}

} // namespace pubsub
} // namespace marlin

#endif // MARLIN_PUBSUB_PUBSUBCLIENT_HPP
