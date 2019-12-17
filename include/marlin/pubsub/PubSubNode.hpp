/*! \file PubSubNode.hpp
    \brief Containing provisions for Publish Subscribe (PubSub) Server functionality
*/

#ifndef MARLIN_PUBSUB_PUBSUBNODE_HPP
#define MARLIN_PUBSUB_PUBSUBNODE_HPP

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

namespace lpf {
template<typename Delegate>
struct IsTransportEncrypted<stream::StreamTransport<
	Delegate,
	net::UdpTransport
>> {
	constexpr static bool value = true;
};
}

namespace pubsub {

//! Class containing the Pub-Sub functionality
/*!
	Uses the custom marlin-StreamTransport for message delivery

	Important functions:
	\li subscribe(publisher_address)
	\li unsubsribe(publisher_address)
	\li send_message_on_channel(channel, message)
*/
template<
	typename PubSubDelegate,
	bool enable_cut_through = false,
	bool accept_unsol_conn = false,
	bool enable_relay = false
>
class PubSubNode {
private:
	size_t max_sol_conns;
	static constexpr uint64_t DefaultMsgIDTimerInterval = 10000;
	static constexpr uint64_t DefaultPeerSelectTimerInterval = 60000;

//---------------- Transport types ----------------//
public:
	using Self = PubSubNode<
		PubSubDelegate,
		enable_cut_through,
		accept_unsol_conn,
		enable_relay
	>;

	template<typename ListenDelegate, typename TransportDelegate>
	using BaseStreamTransportFactory = stream::StreamTransportFactory<
		ListenDelegate,
		TransportDelegate,
		net::UdpTransportFactory,
		net::UdpTransport
	>;
	template<typename Delegate>
	using BaseStreamTransport = stream::StreamTransport<
		Delegate,
		net::UdpTransport
	>;

	using BaseTransportFactory = lpf::LpfTransportFactory<
		Self,
		Self,
		BaseStreamTransportFactory,
		BaseStreamTransport,
		enable_cut_through
	>;
	using BaseTransport = lpf::LpfTransport<
		Self,
		BaseStreamTransport,
		enable_cut_through
	>;
private:

//---------------- Subscription management ----------------//
public:
	typedef PubSubTransportSet<BaseTransport> TransportSet;
	typedef std::unordered_map<std::string, TransportSet> TransportSetMap;

	TransportSetMap channel_subscriptions;
	TransportSetMap potential_channel_subscriptions;

	TransportSet sol_conns;
	TransportSet sol_standby_conns;
	TransportSet unsol_conns;
	// TransportSet unsol_standby_conns;

	void send_SUBSCRIBE(BaseTransport &transport, std::string const channel);
	void send_UNSUBSCRIBE(BaseTransport &transport, std::string const channel);

	bool add_sol_conn(net::SocketAddress const &addr);
	bool add_sol_conn(BaseTransport &transport);
	bool add_sol_standby_conn(BaseTransport &transport);
	bool add_unsol_conn(BaseTransport &transport);
	// bool add_unsol_standby_conn(BaseTransport &transport); TODO: to be introduced later

	bool remove_conn(TransportSet &t_set, BaseTransport &Transport);

	int get_num_active_subscribers(std::string channel);
	void add_subscriber_to_channel(std::string channel, BaseTransport &transport);
	void add_subscriber_to_potential_channel(std::string channel, BaseTransport &transport);
	void remove_subscriber_from_channel(std::string channel, BaseTransport &transport);
	void remove_subscriber_from_potential_channel(std::string channel, BaseTransport &transport);
private:
	uv_timer_t peer_selection_timer;

	static void peer_selection_timer_cb(uv_timer_t *handle) {
		auto &node = *(Self *)handle->data;

		node.delegate->manage_subscriptions(node.max_sol_conns, node.sol_conns, node.sol_standby_conns);

		// std::for_each(
		// 	node.delegate->channels.begin(),
		// 	node.delegate->channels.end(),
		// 	[&] (std::string const channel) {
		// 		node.delegate->manage_subscribers(channel, node.channel_subscriptions[channel], node.potential_channel_subscriptions[channel]);
		// 	}
		// );
	}

//---------------- Pubsub protocol ----------------//
private:
	BaseTransportFactory f;

	void did_recv_SUBSCRIBE(BaseTransport &transport, net::Buffer &&message);

	void did_recv_UNSUBSCRIBE(BaseTransport &transport, net::Buffer &&message);

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
		uint64_t size,
		char const* witness_data,
		uint16_t witness_size
	);

	void did_recv_HEARTBEAT(BaseTransport &transport, net::Buffer &&message);
	void send_HEARTBEAT(BaseTransport &transport);

//---------------- Base layer ----------------//
public:
	// Listen delegate
	bool should_accept(net::SocketAddress const &addr);
	void did_create_transport(BaseTransport &transport);

	// Transport delegate
	void did_dial(BaseTransport &transport);
	void did_recv_message(BaseTransport &transport, net::Buffer &&message);
	void did_send_message(BaseTransport &transport, net::Buffer &&message);
	void did_close(BaseTransport &transport);

	int dial(net::SocketAddress const &addr, uint8_t const* keys = nullptr);

//---------------- Public Interface ----------------//
	PubSubNode(const net::SocketAddress &_addr, size_t max_sol, uint8_t const* keys = nullptr);
	PubSubDelegate *delegate;

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
		net::SocketAddress const *excluded = nullptr,
		char const* witness_data = nullptr,
		uint16_t witness_size = 0
	);
	void send_message_with_cut_through_check(
		BaseTransport *transport,
		std::string channel,
		uint64_t message_id,
		const char *data,
		uint64_t size,
		char const* witness_data,
		uint16_t witness_size
	);

	void subscribe(net::SocketAddress const &addr);
	void unsubscribe(net::SocketAddress const &addr);
private:

//---------------- Message deduplication ----------------//
public:
private:
	std::uniform_int_distribution<uint64_t> message_id_dist;
	std::mt19937_64 message_id_gen;

	// Message id history for deduplication
	std::vector<std::vector<uint64_t>> message_id_events;
	uint8_t message_id_idx = 0;
	std::unordered_set<uint64_t> message_id_set;

	uv_timer_t message_id_timer;

	static void message_id_timer_cb(uv_timer_t *handle) {
		auto &node = *(Self *)handle->data;

		// Overflow behaviour desirable
		node.message_id_idx++;

		for (
			auto iter = node.message_id_events[node.message_id_idx].begin();
			iter != node.message_id_events[node.message_id_idx].end();
			iter = node.message_id_events[node.message_id_idx].erase(iter)
		) {
			node.message_id_set.erase(*iter);
		}

		for (auto* transport : node.sol_conns) {
			node.send_HEARTBEAT(*transport);
		}

		for (auto* transport : node.sol_standby_conns) {
			node.send_HEARTBEAT(*transport);
		}
		// std::for_each(
		// 	node.delegate->channels.begin(),
		// 	node.delegate->channels.end(),
		// 	[&] (std::string const channel) {
		// 		for (auto* transport : node.channel_subscriptions[channel]) {
		// 			node.send_HEARTBEAT(*transport);
		// 		}
		// 		for (auto* pot_transport : node.potential_channel_subscriptions[channel]) {
		// 			node.send_HEARTBEAT(*pot_transport);
		// 		}
		// 	}
		// );
	}

//---------------- Cut through ----------------//
public:
	void cut_through_recv_start(BaseTransport &transport, uint16_t id, uint64_t length);
	void cut_through_recv_bytes(BaseTransport &transport, uint16_t id, net::Buffer &&bytes);
	void cut_through_recv_end(BaseTransport &transport, uint16_t id);
	void cut_through_recv_reset(BaseTransport &transport, uint16_t id);
private:
	struct pairhash {
		template <typename T, typename U>
		std::size_t operator()(const std::pair<T, U> &p) const
		{
			return std::hash<T>()(p.first) ^ std::hash<U>()(p.second);
		}
	};
	std::unordered_map<
		std::pair<BaseTransport *, uint16_t>,
		std::list<std::pair<BaseTransport *, uint16_t>>,
		pairhash
	> cut_through_map;
	std::unordered_map<
		std::pair<BaseTransport *, uint16_t>,
		uint64_t,
		pairhash
	> cut_through_length;
	std::unordered_map<
		std::pair<BaseTransport *, uint16_t>,
		bool,
		pairhash
	> cut_through_header_recv;

	uint8_t const* keys = nullptr;
};


// Impl

//---------------- PubSub functions begin ----------------//

//! Callback on receipt of subscribe request
/*!
	\param transport StreamTransport instance to be added to subsriber list
	\param bytes Buffer containing the channel name to add the subscriber to
*/
template<
	typename PubSubDelegate,
	bool enable_cut_through,
	bool accept_unsol_conn,
	bool enable_relay
>
void PubSubNode<
	PubSubDelegate,
	enable_cut_through,
	accept_unsol_conn,
	enable_relay
>::did_recv_SUBSCRIBE(
	BaseTransport &transport,
	net::Buffer &&bytes
) {
	std::string channel(bytes.data(), bytes.data()+bytes.size());

	SPDLOG_DEBUG(
		"Received subscribe on channel {} from {}",
		channel,
		transport.dst_addr.to_string()
	);

	// add_subscriber_to_channel(channel, transport);
	if (accept_unsol_conn)
		add_unsol_conn(transport);
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
template<
	typename PubSubDelegate,
	bool enable_cut_through,
	bool accept_unsol_conn,
	bool enable_relay
>
void PubSubNode<
	PubSubDelegate,
	enable_cut_through,
	accept_unsol_conn,
	enable_relay
>::send_SUBSCRIBE(
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
template<
	typename PubSubDelegate,
	bool enable_cut_through,
	bool accept_unsol_conn,
	bool enable_relay
>
void PubSubNode<
	PubSubDelegate,
	enable_cut_through,
	accept_unsol_conn,
	enable_relay
>::did_recv_UNSUBSCRIBE(
	BaseTransport &transport,
	net::Buffer &&bytes
) {
	std::string channel(bytes.data(), bytes.data()+bytes.size());

	SPDLOG_DEBUG(
		"Received unsubscribe on channel {} from {}",
		channel,
		transport.dst_addr.to_string()
	);

	// TODO
	remove_conn(unsol_conns, transport);
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
template<
	typename PubSubDelegate,
	bool enable_cut_through,
	bool accept_unsol_conn,
	bool enable_relay
>
void PubSubNode<
	PubSubDelegate,
	enable_cut_through,
	accept_unsol_conn,
	enable_relay
>::send_UNSUBSCRIBE(
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
template<
	typename PubSubDelegate,
	bool enable_cut_through,
	bool accept_unsol_conn,
	bool enable_relay
>
void PubSubNode<
	PubSubDelegate,
	enable_cut_through,
	accept_unsol_conn,
	enable_relay
>::did_recv_RESPONSE(
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
template<
	typename PubSubDelegate,
	bool enable_cut_through,
	bool accept_unsol_conn,
	bool enable_relay
>
void PubSubNode<
	PubSubDelegate,
	enable_cut_through,
	accept_unsol_conn,
	enable_relay
>::send_RESPONSE(
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
	\li relay/forwards the message to other subscriptions on the channel if the enable_relay flag is true
	\li performs message deduplication i.e doesnt relay the message if it has already been relayed recently
*/
template<
	typename PubSubDelegate,
	bool enable_cut_through,
	bool accept_unsol_conn,
	bool enable_relay
>
void PubSubNode<
	PubSubDelegate,
	enable_cut_through,
	accept_unsol_conn,
	enable_relay
>::did_recv_MESSAGE(
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

	auto witness_length = bytes.read_uint16_be(10+channel_length);

	// Check overflow
	if((uint16_t)bytes.size() < 12 + channel_length + witness_length)
		return;

	if(witness_length > 500) {
		SPDLOG_ERROR("Witness too long: {}", witness_length);
		transport.close();
		return;
	}

	// Send it onward
	if(message_id_set.find(message_id) == message_id_set.end()) { // Deduplicate message
		message_id_set.insert(message_id);
		message_id_events[message_id_idx].push_back(message_id);

		char *new_witness = new char[witness_length+32];
		std::memcpy(new_witness, bytes.data()+12+channel_length, witness_length);
		crypto_scalarmult_base((uint8_t*)new_witness+witness_length, keys);

		bytes.cover(12 + channel_length + witness_length);

		if constexpr (enable_relay) {
			send_message_on_channel(
				channel,
				message_id,
				bytes.data(),
				bytes.size(),
				&transport.dst_addr,
				new_witness,
				witness_length + 32
			);
		}

		delete[] new_witness;

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
template<
	typename PubSubDelegate,
	bool enable_cut_through,
	bool accept_unsol_conn,
	bool enable_relay
>
void PubSubNode<
	PubSubDelegate,
	enable_cut_through,
	accept_unsol_conn,
	enable_relay
>::send_MESSAGE(
	BaseTransport &transport,
	std::string channel,
	uint64_t message_id,
	const char *data,
	uint64_t size,
	const char* witness_data,
	uint16_t witness_size
) {
	char *message = new char[11 + channel.size() + 2 + witness_size + size];

	message[0] = 3;

	net::Buffer m(message, 11 + channel.size() + 2 + witness_size + size);
	m.write_uint64_be(1, message_id);
	m.write_uint16_be(9, channel.size());
	std::memcpy(message + 11, channel.data(), channel.size());
	m.write_uint16_be(11 + channel.size(), witness_size);
	std::memcpy(message + 11 + channel.size() + 2, witness_data, witness_size);
	std::memcpy(message + 11 + channel.size() + 2 + witness_size, data, size);

	transport.send(std::move(m));
}

template<
	typename PubSubDelegate,
	bool enable_cut_through,
	bool accept_unsol_conn,
	bool enable_relay
>
void PubSubNode<
	PubSubDelegate,
	enable_cut_through,
	accept_unsol_conn,
	enable_relay
>::send_HEARTBEAT(
	BaseTransport &transport
) {
	char *message = new char[1];

	message[0] = 4;

	net::Buffer m(message, 1);

	transport.send(std::move(m));
}

//---------------- PubSub functions end ----------------//


//---------------- Listen delegate functions begin ----------------//

template<
	typename PubSubDelegate,
	bool enable_cut_through,
	bool accept_unsol_conn,
	bool enable_relay
>
bool PubSubNode<
	PubSubDelegate,
	enable_cut_through,
	accept_unsol_conn,
	enable_relay
>::should_accept(net::SocketAddress const &) {
	return accept_unsol_conn;
}

template<
	typename PubSubDelegate,
	bool enable_cut_through,
	bool accept_unsol_conn,
	bool enable_relay
>
void PubSubNode<
	PubSubDelegate,
	enable_cut_through,
	accept_unsol_conn,
	enable_relay
>::did_create_transport(
	BaseTransport &transport
) {
	SPDLOG_DEBUG(
		"DID CREATE TRANSPORT: {}",
		transport.dst_addr.to_string()
	);

	transport.setup(this, keys);
}

//---------------- Listen delegate functions end ----------------//


//---------------- Transport delegate functions begin ----------------//

template<
	typename PubSubDelegate,
	bool enable_cut_through,
	bool accept_unsol_conn,
	bool enable_relay
>
void PubSubNode<
	PubSubDelegate,
	enable_cut_through,
	accept_unsol_conn,
	enable_relay
>::did_dial(BaseTransport &transport) {

	SPDLOG_DEBUG(
		"DID DIAL: {}",
		transport.dst_addr.to_string()
	);

	std::for_each(
		delegate->channels.begin(),
		delegate->channels.end(),
		[&] (std::string const channel) {
			send_SUBSCRIBE(transport, channel);
		}
	);

	add_sol_conn(transport);
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
template<
	typename PubSubDelegate,
	bool enable_cut_through,
	bool accept_unsol_conn,
	bool enable_relay
>
void PubSubNode<
	PubSubDelegate,
	enable_cut_through,
	accept_unsol_conn,
	enable_relay
>::did_recv_message(
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
		// HEARTBEAT, ignore
		case 4:
		break;
	}
}

template<
	typename PubSubDelegate,
	bool enable_cut_through,
	bool accept_unsol_conn,
	bool enable_relay
>
void PubSubNode<
	PubSubDelegate,
	enable_cut_through,
	accept_unsol_conn,
	enable_relay
>::did_send_message(
	BaseTransport &,
	net::Buffer &&
) {}

template<
	typename PubSubDelegate,
	bool enable_cut_through,
	bool accept_unsol_conn,
	bool enable_relay
>
void PubSubNode<
	PubSubDelegate,
	enable_cut_through,
	accept_unsol_conn,
	enable_relay
>::did_close(BaseTransport &transport) {
	// Remove from subscribers
	std::for_each(
		delegate->channels.begin(),
		delegate->channels.end(),
		[&] (std::string const channel) {
			channel_subscriptions[channel].erase(&transport);
			potential_channel_subscriptions[channel].erase(&transport);
		}
	);

	// Flush subscribers
	for(auto id : transport.cut_through_used_ids) {
		for(auto& [subscriber, subscriber_id] : cut_through_map[std::make_pair(&transport, id)]) {
			subscriber->cut_through_send_reset(subscriber_id);
		}

		cut_through_map.erase(std::make_pair(&transport, id));
	}

	// Remove subscriptions
	for(auto& [_, subscribers] : cut_through_map) {
		for (auto iter = subscribers.begin(); iter != subscribers.end();) {
			if(iter->first == &transport) {
				iter = subscribers.erase(iter);
			} else {
				iter++;
			}
		}
	}
}

//---------------- Transport delegate functions end ----------------//


template<
	typename PubSubDelegate,
	bool enable_cut_through,
	bool accept_unsol_conn,
	bool enable_relay
>
PubSubNode<
	PubSubDelegate,
	enable_cut_through,
	accept_unsol_conn,
	enable_relay
>::PubSubNode(
	const net::SocketAddress &addr,
	size_t max_sol,
	uint8_t const* keys
) : max_sol_conns(max_sol),
	message_id_gen(std::random_device()()),
	message_id_events(256),
	keys(keys)
{
	f.bind(addr);
	f.listen(*this);


	SPDLOG_DEBUG(
		"PUBSUB LISTENING ON : {}",
		addr.to_string()
	);

	uv_timer_init(uv_default_loop(), &message_id_timer);
	this->message_id_timer.data = (void *)this;
	uv_timer_start(&message_id_timer, &message_id_timer_cb, DefaultMsgIDTimerInterval, DefaultMsgIDTimerInterval);

	uv_timer_init(uv_default_loop(), &peer_selection_timer);
	this->peer_selection_timer.data = (void *)this;
	uv_timer_start(&peer_selection_timer, &peer_selection_timer_cb, DefaultPeerSelectTimerInterval, DefaultPeerSelectTimerInterval);
}

template<
	typename PubSubDelegate,
	bool enable_cut_through,
	bool accept_unsol_conn,
	bool enable_relay
>
int PubSubNode<
	PubSubDelegate,
	enable_cut_through,
	accept_unsol_conn,
	enable_relay
>::dial(net::SocketAddress const &addr, uint8_t const* keys) {
	SPDLOG_DEBUG(
		"SENDING DIAL TO: {}",
		addr.to_string()
	);

	return f.dial(addr, *this, keys);
}


//! sends messages over a given channel
/*!
	\param channel name of channel to send message on
	\param data char* byte sequence of message to send
	\param size of the message to send
	\param excluded avoid this address while sending the message
*/
template<
	typename PubSubDelegate,
	bool enable_cut_through,
	bool accept_unsol_conn,
	bool enable_relay
>
uint64_t PubSubNode<
	PubSubDelegate,
	enable_cut_through,
	accept_unsol_conn,
	enable_relay
>::send_message_on_channel(
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
template<
	typename PubSubDelegate,
	bool enable_cut_through,
	bool accept_unsol_conn,
	bool enable_relay
>
void PubSubNode<
	PubSubDelegate,
	enable_cut_through,
	accept_unsol_conn,
	enable_relay
>::send_message_on_channel(
	std::string channel,
	uint64_t message_id,
	const char *data,
	uint64_t size,
	net::SocketAddress const *excluded,
	char const* witness_data,
	uint16_t witness_size
) {
	if(witness_data == nullptr) {
		witness_size = 32;
	}

	for (
		auto it = sol_conns.begin();
		it != sol_conns.end();
		it++
	) {
		// Exclude given address, usually sender tp prevent loops
		if(excluded != nullptr && (*it)->dst_addr == *excluded)
			continue;
		send_message_with_cut_through_check(*it, channel, message_id, data, size, witness_data, witness_size);
	}

	for (
		auto it = unsol_conns.begin();
		it != unsol_conns.end();
		it++
	) {
		// Exclude given address, usually sender tp prevent loops
		if(excluded != nullptr && (*it)->dst_addr == *excluded)
			continue;
		send_message_with_cut_through_check(*it, channel, message_id, data, size, witness_data, witness_size);
	}
}


template<
	typename PubSubDelegate,
	bool enable_cut_through,
	bool accept_unsol_conn,
	bool enable_relay
>
void PubSubNode<
	PubSubDelegate,
	enable_cut_through,
	accept_unsol_conn,
	enable_relay
>::send_message_with_cut_through_check(
	BaseTransport *transport,
	std::string channel,
	uint64_t message_id,
	const char *data,
	uint64_t size,
	char const* witness_data,
	uint16_t witness_size
) {
	SPDLOG_DEBUG(
		"Sending message {} on channel {} to {}",
		message_id,
		channel,
		transport->dst_addr.to_string()
	);

	if(size > 50000) {
		char *message = new char[11 + channel.size() + 2 + witness_size + size];

		message[0] = 3;

		net::Buffer m(message, 11 + channel.size() + 2 + witness_size + size);
		m.write_uint64_be(1, message_id);
		m.write_uint16_be(9, channel.size());
		std::memcpy(message + 11, channel.data(), channel.size());
		m.write_uint16_be(11 + channel.size(), witness_size);
		if(witness_data == nullptr) {
			crypto_scalarmult_base((uint8_t*)message + 11 + channel.size() + 2, keys);
		} else {
			std::memcpy(message + 11 + channel.size() + 2, witness_data, witness_size);
		}
		std::memcpy(message + 11 + channel.size() + 2 + witness_size, data, size);

		auto res = transport->cut_through_send(std::move(m));

		// TODO: Handle better
		if(res < 0) {
			SPDLOG_ERROR("Cut through send failed");
			transport->close();
		}
	} else {
		send_MESSAGE(*transport, channel, message_id, data, size, witness_data, witness_size);
	}
}

//! subscribes to given publisher
/*!
	\param addr publisher address to subscribe to
*/
template<
	typename PubSubDelegate,
	bool enable_cut_through,
	bool accept_unsol_conn,
	bool enable_relay
>
void PubSubNode<
	PubSubDelegate,
	enable_cut_through,
	accept_unsol_conn,
	enable_relay
>::subscribe(net::SocketAddress const &addr) {
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

//! unsubscribes from given publisher
/*!
	\param addr publisher address to unsubscribe from
*/
template<
	typename PubSubDelegate,
	bool enable_cut_through,
	bool accept_unsol_conn,
	bool enable_relay
>
void PubSubNode<
	PubSubDelegate,
	enable_cut_through,
	accept_unsol_conn,
	enable_relay
>::unsubscribe(net::SocketAddress const &addr) {
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

template<
	typename PubSubDelegate,
	bool enable_cut_through,
	bool accept_unsol_conn,
	bool enable_relay
>
bool PubSubNode<
	PubSubDelegate,
	enable_cut_through,
	accept_unsol_conn,
	enable_relay
>::add_sol_conn(net::SocketAddress const &addr) {

	auto *transport = f.get_transport(addr);

	if(transport == nullptr) {
		return false;
	}

	add_sol_conn(*transport);
}

template<
	typename PubSubDelegate,
	bool enable_cut_through,
	bool accept_unsol_conn,
	bool enable_relay
>
bool PubSubNode<
	PubSubDelegate,
	enable_cut_through,
	accept_unsol_conn,
	enable_relay
>::add_sol_conn(BaseTransport &transport) {

	//TODO: size check.
	if (sol_conns.size() >= max_sol_conns) {
		add_sol_standby_conn(transport);
		return false;
	}

	if (unsol_conns.check_tranport_in_set(transport)) {
		remove_conn(unsol_conns, transport);
	}

	if (!sol_conns.check_tranport_in_set(transport) &&
		!sol_standby_conns.check_tranport_in_set(transport)) {

		SPDLOG_DEBUG("Adding address: {} to sol conn list",
			transport.dst_addr.to_string()
		);

		sol_conns.insert(&transport);
		//TODO: send response
		send_RESPONSE(transport, true, "SUBSCRIBED");

		return true;
	}

	return false;
}

template<
	typename PubSubDelegate,
	bool enable_cut_through,
	bool accept_unsol_conn,
	bool enable_relay
>
bool PubSubNode<
	PubSubDelegate,
	enable_cut_through,
	accept_unsol_conn,
	enable_relay
>::add_sol_standby_conn(BaseTransport &transport) {

	if(!sol_conns.check_tranport_in_set(transport) &&
	   !sol_standby_conns.check_tranport_in_set(transport)) {

		SPDLOG_DEBUG("Adding address: {} to sol standby conn list",
			transport.dst_addr.to_string()
		);

		sol_standby_conns.insert(&transport);
		return true;
	}

	return false;
}

template<
	typename PubSubDelegate,
	bool enable_cut_through,
	bool accept_unsol_conn,
	bool enable_relay
>
bool PubSubNode<
	PubSubDelegate,
	enable_cut_through,
	accept_unsol_conn,
	enable_relay
>::add_unsol_conn(BaseTransport &transport) {

	if(!sol_conns.check_tranport_in_set(transport) &&
	   !sol_standby_conns.check_tranport_in_set(transport) &&
	   !unsol_conns.check_tranport_in_set(transport)) {

		SPDLOG_DEBUG("Adding address: {} to unsol conn list",
			transport.dst_addr.to_string()
		);

		unsol_conns.insert(&transport);
		return true;
	}

	return false;
}

template<
	typename PubSubDelegate,
	bool enable_cut_through,
	bool accept_unsol_conn,
	bool enable_relay
>
bool PubSubNode<
	PubSubDelegate,
	enable_cut_through,
	accept_unsol_conn,
	enable_relay
>::remove_conn(TransportSet &t_set, BaseTransport &transport) {

	if (t_set.check_tranport_in_set(transport)) {
		SPDLOG_DEBUG("Removing address: {} from list",
			transport.dst_addr.to_string()
		);

		t_set.erase(&transport);

		//TODO Send response
		if (&t_set == &sol_conns) {
			send_RESPONSE(transport, true, "UNSUBSCRIBED");
		}

		return true;
	}

	return false;
}


template<
	typename PubSubDelegate,
	bool enable_cut_through,
	bool accept_unsol_conn,
	bool enable_relay
>
int PubSubNode<
	PubSubDelegate,
	enable_cut_through,
	accept_unsol_conn,
	enable_relay
>::get_num_active_subscribers(
	std::string channel) {
	return channel_subscriptions[channel].size();
}

template<
	typename PubSubDelegate,
	bool enable_cut_through,
	bool accept_unsol_conn,
	bool enable_relay
>
void PubSubNode<
	PubSubDelegate,
	enable_cut_through,
	accept_unsol_conn,
	enable_relay
>::add_subscriber_to_channel(
	std::string channel,
	BaseTransport &transport
) {
	if (!channel_subscriptions[channel].check_tranport_in_set(transport)) {
		if (channel_subscriptions[channel].size() >= max_sol_conns) {
			add_subscriber_to_potential_channel(channel, transport);
			return;
		}

		SPDLOG_DEBUG("Adding address: {} to subscribers list on channel: {} ",
			transport.dst_addr.to_string(),
			channel
		);
		channel_subscriptions[channel].insert(&transport);

		send_RESPONSE(transport, true, "SUBSCRIBED TO " + channel);
	}
}

template<
	typename PubSubDelegate,
	bool enable_cut_through,
	bool accept_unsol_conn,
	bool enable_relay
>
void PubSubNode<
	PubSubDelegate,
	enable_cut_through,
	accept_unsol_conn,
	enable_relay
>::add_subscriber_to_potential_channel(
	std::string channel,
	BaseTransport &transport) {

	if (!potential_channel_subscriptions[channel].check_tranport_in_set(transport)) {
		SPDLOG_DEBUG("Adding address: {} to potential subscribers list on channel: {} ",
			transport.dst_addr.to_string(),
			channel);
		potential_channel_subscriptions[channel].insert(&transport);
	}
}

template<
	typename PubSubDelegate,
	bool enable_cut_through,
	bool accept_unsol_conn,
	bool enable_relay
>
void PubSubNode<
	PubSubDelegate,
	enable_cut_through,
	accept_unsol_conn,
	enable_relay
>::remove_subscriber_from_channel(
	std::string channel,
	BaseTransport &transport) {

	if (channel_subscriptions[channel].check_tranport_in_set(transport)) {
		SPDLOG_DEBUG("Removing address: {} from subscribers list on channel: {} ",
			transport.dst_addr.to_string(),
			channel
		);
		channel_subscriptions[channel].erase(&transport);

		// Send response
		send_RESPONSE(transport, true, "UNSUBSCRIBED FROM " + channel);
	}
}

template<
	typename PubSubDelegate,
	bool enable_cut_through,
	bool accept_unsol_conn,
	bool enable_relay
>
void PubSubNode<
	PubSubDelegate,
	enable_cut_through,
	accept_unsol_conn,
	enable_relay
>::remove_subscriber_from_potential_channel(
	std::string channel,
	BaseTransport &transport
) {
	if (potential_channel_subscriptions[channel].check_tranport_in_set(transport)) {
		SPDLOG_DEBUG("Removing address: {} from potential subscribers list on channel: {} ",
			transport.dst_addr.to_string(),
			channel
		);
		potential_channel_subscriptions[channel].erase(&transport);
	}
}


template<
	typename PubSubDelegate,
	bool enable_cut_through,
	bool accept_unsol_conn,
	bool enable_relay
>
void PubSubNode<
	PubSubDelegate,
	enable_cut_through,
	accept_unsol_conn,
	enable_relay
>::cut_through_recv_start(
	BaseTransport &transport,
	uint16_t id,
	uint64_t length
) {
	cut_through_map[std::make_pair(&transport, id)] = {};
	cut_through_header_recv[std::make_pair(&transport, id)] = false;
	cut_through_length[std::make_pair(&transport, id)] = length;
}

template<
	typename PubSubDelegate,
	bool enable_cut_through,
	bool accept_unsol_conn,
	bool enable_relay
>
void PubSubNode<
	PubSubDelegate,
	enable_cut_through,
	accept_unsol_conn,
	enable_relay
>::cut_through_recv_bytes(
	BaseTransport &transport,
	uint16_t id,
	net::Buffer &&bytes
) {
	if(!cut_through_header_recv[std::make_pair(&transport, id)]) {
		auto channel_length = bytes.read_uint16_be(9);

		// Check overflow
		if((uint16_t)bytes.size() < 11 + channel_length) {
			SPDLOG_ERROR("Not enough header: {}, {}", bytes.size(), channel_length);
			transport.close();
			return;
		}

		auto witness_length = bytes.read_uint16_be(11+channel_length);

		bool found = false;
		for(uint i = 0; i < witness_length/32; i++) {
			if(std::memcmp(bytes.data() + 13 + channel_length + 32*i, transport.get_remote_static_pk(), 32) == 0) {
				found = true;
			}
		}

		if((uint16_t)bytes.size() < 13 + channel_length + witness_length) {
			SPDLOG_ERROR("Not enough header: {}, {}", bytes.size(), witness_length);
			transport.close();
			return;
		}

		if(channel_length > 10) {
			SPDLOG_ERROR("Channel too long: {}", channel_length);
			transport.close();
			return;
		}

		cut_through_header_recv[std::make_pair(&transport, id)] = true;

		auto channel = std::string(bytes.data()+11, bytes.data()+11+channel_length);
		for(auto *subscriber : channel_subscriptions[channel]) {
			auto sub_id = subscriber->cut_through_send_start(
				cut_through_length[std::make_pair(&transport, id)]
			);
			if(sub_id == 0) {
				SPDLOG_ERROR("Cannot send to subscriber");
				continue;
			}

			cut_through_map[std::make_pair(&transport, id)].push_back(
				std::make_pair(subscriber, sub_id)
			);
		}

		cut_through_recv_bytes(transport, id, std::move(bytes));
	} else {
		for(auto [subscriber, sub_id] : cut_through_map[std::make_pair(&transport, id)]) {
			if(&transport == subscriber) continue;

			auto sub_bytes = net::Buffer(new char[bytes.size()], bytes.size());
			std::memcpy(sub_bytes.data(), bytes.data(), bytes.size());

			auto res = subscriber->cut_through_send_bytes(sub_id, std::move(sub_bytes));

			// TODO: Handle better
			if(res < 0) {
				SPDLOG_ERROR("Cut through send failed");
				subscriber->close();
			}
		}
	}
}

template<
	typename PubSubDelegate,
	bool enable_cut_through,
	bool accept_unsol_conn,
	bool enable_relay
>
void PubSubNode<
	PubSubDelegate,
	enable_cut_through,
	accept_unsol_conn,
	enable_relay
>::cut_through_recv_end(
	BaseTransport &transport,
	uint16_t id
) {
	for(auto [subscriber, sub_id] : cut_through_map[std::make_pair(&transport, id)]) {
		subscriber->cut_through_send_end(sub_id);
	}
}

template<
	typename PubSubDelegate,
	bool enable_cut_through,
	bool accept_unsol_conn,
	bool enable_relay
>
void PubSubNode<
	PubSubDelegate,
	enable_cut_through,
	accept_unsol_conn,
	enable_relay
>::cut_through_recv_reset(
	BaseTransport &transport,
	uint16_t id
) {
	for(auto [subscriber, sub_id] : cut_through_map[std::make_pair(&transport, id)]) {
		subscriber->cut_through_send_reset(sub_id);
	}
}

} // namespace pubsub
} // namespace marlin

#endif // MARLIN_PUBSUB_PUBSUBNODE_HPP
