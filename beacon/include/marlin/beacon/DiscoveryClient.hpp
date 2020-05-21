/*! \file DiscoveryClient.hpp
	\brief Client side implementation of beacon functionality to discover peer nodes to connect to
*/

#ifndef MARLIN_BEACON_DISCOVERYCLIENT_HPP
#define MARLIN_BEACON_DISCOVERYCLIENT_HPP

#include <marlin/asyncio/core/Timer.hpp>
#include <marlin/asyncio/udp/UdpTransportFactory.hpp>
#include <map>

#include <sodium.h>

#include "Messages.hpp"


namespace marlin {
namespace beacon {

//! Class implementing the client side node discovery functionality
/*!
	Features:
	\li Uses the custom marlin UDPTransport for message delivery
	\li HEARTBEAT - function to register with the central discovery server to enable oneself be discoverable
	\li DISCPEER - function to find other discoverable clients
	\li DISCPROTO - function to find supported protocols on a node
*/
template<
	typename DiscoveryClientDelegate,
	template<typename, typename> class TransportFactory = asyncio::UdpTransportFactory,
	template<typename> class Transport = asyncio::UdpTransport
>
class DiscoveryClient {
private:
	using Self = DiscoveryClient<
		DiscoveryClientDelegate,
		TransportFactory,
		Transport
	>;

	using BaseTransportFactory = TransportFactory<Self, Self>;
	using BaseTransport = Transport<Self>;
	using BaseMessageType = typename BaseTransport::MessageType;

	BaseTransportFactory f;

	// Discovery protocol
	void send_DISCPROTO(BaseTransport &transport);
	void did_recv_DISCPROTO(BaseTransport &transport);
	void send_LISTPROTO(BaseTransport &transport);
	void did_recv_LISTPROTO(BaseTransport &transport, LISTPROTO<BaseMessageType> &&packet);

	void send_DISCPEER(BaseTransport &transport);
	void did_recv_LISTPEER(BaseTransport &transport, LISTPEER<BaseMessageType> &&packet);

	void send_HEARTBEAT(BaseTransport &transport);

	/*!
		stores the UDP Transport connection instance to the beacon server
	*/
	BaseTransport *beacon = nullptr;

	void beacon_timer_cb();
	asyncio::Timer beacon_timer;

	void heartbeat_timer_cb();
	asyncio::Timer heartbeat_timer;

public:
	// Listen delegate
	bool should_accept(core::SocketAddress const &addr);
	void did_create_transport(BaseTransport &transport);

	// Transport delegate
	void did_dial(BaseTransport &transport);
	void did_recv_packet(BaseTransport &transport, core::Buffer &&packet);
	void did_send_packet(BaseTransport &transport, core::Buffer &&packet);

	template<typename ...Args>
	DiscoveryClient(
		core::SocketAddress const &addr,
		uint8_t const* static_sk,
		Args&&... args
	);
	~DiscoveryClient();

	DiscoveryClientDelegate *delegate;
	bool is_discoverable = false;

	void start_discovery(core::SocketAddress const &beacon_addr);

	void close();

private:
	uint8_t static_sk[crypto_box_SECRETKEYBYTES];
	uint8_t static_pk[crypto_box_PUBLICKEYBYTES];

	std::unordered_map<core::SocketAddress, std::array<uint8_t, 32>> node_key_map;
};


// Impl

//---------------- Helper macros begin ----------------//

#define DISCOVERYCLIENT_TEMPLATE typename DiscoveryClientDelegate, \
	template<typename, typename> class TransportFactory, \
	template<typename> class Transport

#define DISCOVERYCLIENT DiscoveryClient< \
	DiscoveryClientDelegate, \
	TransportFactory, \
	Transport \
>

//---------------- Helper macros end ----------------//


//---------------- Discovery protocol functions begin ----------------//


/*!
	function to get list of supported protocols on a client node
*/
template<DISCOVERYCLIENT_TEMPLATE>
void DISCOVERYCLIENT::send_DISCPROTO(
	BaseTransport &transport
) {
	transport.send(DISCPROTO<BaseMessageType>());
}


/*!
	\li Callback on receipt of disc proto
	\li Sends back the protocols supported on this node
*/
template<DISCOVERYCLIENT_TEMPLATE>
void DISCOVERYCLIENT::did_recv_DISCPROTO(
	BaseTransport &transport
) {
	SPDLOG_DEBUG("DISCPROTO <<< {}", transport.dst_addr.to_string());

	send_LISTPROTO(transport);
}


/*!
	sends the list of supported protocols on this node
*/
template<DISCOVERYCLIENT_TEMPLATE>
void DISCOVERYCLIENT::send_LISTPROTO(
	BaseTransport &transport
) {
	auto protocols = delegate->get_protocols();
	assert(protocols.size() < 100);

	transport.send(
		LISTPROTO<BaseMessageType>(protocols.size())
		.set_protocols(protocols.begin(), protocols.end())
	);
}

template<DISCOVERYCLIENT_TEMPLATE>
void DISCOVERYCLIENT::did_recv_LISTPROTO(
	BaseTransport &transport,
	LISTPROTO<BaseMessageType> &&packet
) {
	SPDLOG_DEBUG("LISTPROTO <<< {}", transport.dst_addr.to_string());

	if(!packet.validate()) {
		return;
	}

	for(auto iter = packet.protocols_begin(); iter != packet.protocols_end(); ++iter) {
		auto [protocol, version, port] = *iter;
		core::SocketAddress peer_addr(transport.dst_addr);
		peer_addr.set_port(port);

		delegate->new_peer(peer_addr, node_key_map[transport.dst_addr].data(), protocol, version);
	}
}

/*!
	sends peer discovery message
*/
template<DISCOVERYCLIENT_TEMPLATE>
void DISCOVERYCLIENT::send_DISCPEER(
	BaseTransport &transport
) {
	transport.send(DISCPEER<BaseMessageType>());
}

/*!
	\li Callback on receipt of list peers
	\li Tries to connect to each of the peers via dial
*/
template<DISCOVERYCLIENT_TEMPLATE>
void DISCOVERYCLIENT::did_recv_LISTPEER(
	BaseTransport &transport [[maybe_unused]],
	LISTPEER<BaseMessageType> &&packet
) {
	SPDLOG_DEBUG("LISTPEER <<< {}", transport.dst_addr.to_string());

	if(!packet.validate()) {
		return;
	}

	for(auto iter = packet.peers_begin(); iter != packet.peers_end(); ++iter) {
		auto [peer_addr, key] = *iter;
		node_key_map[peer_addr] = key;

		f.dial(peer_addr, *this);
	}
}

/*!
	sends heartbeat message to refresh/create entry at the discovery server to keep the node discoverable
*/
template<DISCOVERYCLIENT_TEMPLATE>
void DISCOVERYCLIENT::send_HEARTBEAT(
	BaseTransport &transport
) {
	transport.send(HEARTBEAT<BaseMessageType>().set_key(static_pk));
}

/*!
	callback to periodically send DISCPEER sending in search of new peers
*/
template<DISCOVERYCLIENT_TEMPLATE>
void DISCOVERYCLIENT::beacon_timer_cb() {
	// Discover new peers
	send_DISCPEER(*beacon);
}

/*!
	callback to periodically send HEARTBEAT to refresh the entry at beacon server
*/
template<DISCOVERYCLIENT_TEMPLATE>
void DISCOVERYCLIENT::heartbeat_timer_cb() {
	if(is_discoverable) {
		send_HEARTBEAT(*beacon);
	}
}

//---------------- Discovery protocol functions begin ----------------//


//---------------- Listen delegate functions begin ----------------//

template<DISCOVERYCLIENT_TEMPLATE>
bool DISCOVERYCLIENT::should_accept(
	core::SocketAddress const &
) {
	return is_discoverable;
}

template<DISCOVERYCLIENT_TEMPLATE>
void DISCOVERYCLIENT::did_create_transport(
	BaseTransport &transport
) {
	transport.setup(this);
}

//---------------- Listen delegate functions end ----------------//


//---------------- Transport delegate functions begin ----------------//

template<DISCOVERYCLIENT_TEMPLATE>
void DISCOVERYCLIENT::did_dial(
	BaseTransport &transport
) {
	if(beacon == nullptr) {
		beacon = &transport;
		beacon_timer.template start<Self, &Self::beacon_timer_cb>(0, 60000);
		if(is_discoverable) {
			heartbeat_timer.template start<Self, &Self::heartbeat_timer_cb>(0, 10000);
		}
	} else {
		send_DISCPROTO(transport);
	}
}

//! receives the packet and processes them
/*!
	Determines the type of packet by reading the first byte and redirects the packet to appropriate function for further processing

	\li \b first-byte	:	\b type
	\li 0			:	DISCPROTO
	\li 1			:	LISTPROTO
	\li 2			:	ERROR - DISCPEER, meant for server
	\li 3			:	LISTPEER
	\li 4			:	ERROR- HEARTBEAT, meant for server
*/
template<DISCOVERYCLIENT_TEMPLATE>
void DISCOVERYCLIENT::did_recv_packet(
	BaseTransport &transport,
	core::Buffer &&packet
) {
	auto type = packet.read_uint8(1);
	if(type == std::nullopt || packet.read_uint8_unsafe(0) != 0) {
		return;
	}

	switch(type.value()) {
		// DISCPROTO
		case 0: did_recv_DISCPROTO(transport);
		break;
		// LISTPROTO
		case 1: did_recv_LISTPROTO(transport, std::move(packet));
		break;
		// DISCOVER
		case 2: SPDLOG_ERROR("Unexpected DISCPEER from {}", transport.dst_addr.to_string());
		break;
		// PEERLIST
		case 3: did_recv_LISTPEER(transport, std::move(packet));
		break;
		// HEARTBEAT
		case 4: SPDLOG_ERROR("Unexpected HEARTBEAT from {}", transport.dst_addr.to_string());
		break;
		// UNKNOWN
		default: SPDLOG_TRACE("UNKNOWN <<< {}", transport.dst_addr.to_string());
		break;
	}
}

template<DISCOVERYCLIENT_TEMPLATE>
void DISCOVERYCLIENT::did_send_packet(
	BaseTransport &transport [[maybe_unused]],
	core::Buffer &&packet
) {
	auto type = packet.read_uint8(1);
	if(type == std::nullopt || packet.read_uint8_unsafe(0) != 0) {
		return;
	}

	switch(type.value()) {
		// DISCPROTO
		case 0: SPDLOG_TRACE("DISCPROTO >>> {}", transport.dst_addr.to_string());
		break;
		// LISTPROTO
		case 1: SPDLOG_TRACE("LISTPROTO >>> {}", transport.dst_addr.to_string());
		break;
		// DISCPEER
		case 2: SPDLOG_TRACE("DISCPEER >>> {}", transport.dst_addr.to_string());
		break;
		// LISTPEER
		case 3: SPDLOG_TRACE("LISTPEER >>> {}", transport.dst_addr.to_string());
		break;
		// HEARTBEAT
		case 4: SPDLOG_TRACE("HEARTBEAT >>> {}", transport.dst_addr.to_string());
		break;
		// UNKNOWN
		default: SPDLOG_TRACE("UNKNOWN >>> {}", transport.dst_addr.to_string());
		break;
	}
}

//---------------- Transport delegate functions end ----------------//

template<DISCOVERYCLIENT_TEMPLATE>
template<typename ...Args>
DISCOVERYCLIENT::DiscoveryClient(
	core::SocketAddress const &addr,
	uint8_t const* static_sk,
	Args&&... args
) : f(std::forward<Args>(args)...), beacon_timer(this), heartbeat_timer(this) {
	f.bind(addr);
	f.listen(*this);

	if(sodium_init() == -1) {
		throw;
	}

	std::memcpy(this->static_sk, static_sk, crypto_box_SECRETKEYBYTES);
	crypto_scalarmult_base(this->static_pk, this->static_sk);
}

template<DISCOVERYCLIENT_TEMPLATE>
DISCOVERYCLIENT::~DiscoveryClient() {
	// Explicitly zero to protect memory
	sodium_memzero(static_sk, crypto_box_SECRETKEYBYTES);
}
/*!
	connects to the beacon server to start the peer discovery
*/
template<DISCOVERYCLIENT_TEMPLATE>
void DISCOVERYCLIENT::start_discovery(
	const core::SocketAddress &beacon_addr
) {
	f.dial(beacon_addr, *this);
}

template<DISCOVERYCLIENT_TEMPLATE>
void DISCOVERYCLIENT::close() {
	beacon_timer.stop();
	heartbeat_timer.stop();
}


//---------------- Helper macros undef begin ----------------//

#undef DISCOVERYCLIENT_TEMPLATE
#undef DISCOVERYCLIENT

//---------------- Helper macros undef end ----------------//

} // namespace beacon
} // namespace marlin

#endif // MARLIN_BEACON_DISCOVERYCLIENT_HPP
