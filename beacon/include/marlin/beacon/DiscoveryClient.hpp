/*! \file DiscoveryClient.hpp
	\brief Client side implementation of beacon functionality to discover peer nodes to connect to
*/

#ifndef MARLIN_BEACON_DISCOVERYCLIENT_HPP
#define MARLIN_BEACON_DISCOVERYCLIENT_HPP

#include <marlin/asyncio/core/Timer.hpp>
#include <marlin/asyncio/udp/UdpFiber.hpp>
#include <marlin/core/fibers/VersioningFiber.hpp>
#include <marlin/core/fabric/Fabric.hpp>
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
	template<typename> typename FiberTemplate = core::FabricF<asyncio::UdpFiber, core::VersioningFiber>::type
>
class DiscoveryClient {
private:
	using Self = DiscoveryClient<
		DiscoveryClientDelegate,
		FiberTemplate
	>;
	using FiberType = FiberTemplate<Self&>;

	using BaseMessageType = typename FiberType::InnerMessageType;
	using DISCPROTO = DISCPROTOWrapper<BaseMessageType>;
	using LISTPROTO = LISTPROTOWrapper<BaseMessageType>;
	using DISCPEER = DISCPEERWrapper<BaseMessageType>;
	using LISTPEER = LISTPEERWrapper<BaseMessageType>;
	using HEARTBEAT = HEARTBEATWrapper<BaseMessageType>;

	FiberType fiber;

	// Discovery protocol
	void send_DISCPROTO(FiberType& fiber, core::SocketAddress addr);
	void did_recv_DISCPROTO(FiberType& fiber, core::SocketAddress addr);
	void send_LISTPROTO(FiberType& fiber, core::SocketAddress addr);
	void did_recv_LISTPROTO(FiberType& fiber, LISTPROTO &&packet, core::SocketAddress addr);

	void send_DISCPEER(FiberType& fiber, core::SocketAddress addr);
	void did_recv_LISTPEER(FiberType& fiber, LISTPEER &&packet, core::SocketAddress addr);

	void send_HEARTBEAT(FiberType& fiber, core::SocketAddress addr);

	void did_recv_DISCADDR(FiberType& fiber, core::SocketAddress addr);

	std::vector<core::SocketAddress> discovery_addrs;
	std::vector<core::SocketAddress> heartbeat_addrs;

	void beacon_timer_cb();
	asyncio::Timer beacon_timer;

	void heartbeat_timer_cb();
	asyncio::Timer heartbeat_timer;

public:
	// Transport delegate
	int did_dial(FiberType& fiber, core::SocketAddress addr, size_t type = 0);
	int did_recv(FiberType& fiber, BaseMessageType &&packet, core::SocketAddress addr);
	int did_send(FiberType& fiber, core::Buffer &&packet);

	DiscoveryClient(
		core::SocketAddress const& addr,
		uint8_t const* static_sk
	);
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
	void start_discovery(
		std::vector<core::SocketAddress>&& discovery_addrs,
		std::vector<core::SocketAddress>&& heartbeat_addrs
	);

	void close();

	std::string address;
	std::string name;
private:
	uint8_t static_sk[crypto_box_SECRETKEYBYTES];
	uint8_t static_pk[crypto_box_PUBLICKEYBYTES];

	std::unordered_map<core::SocketAddress, std::array<uint8_t, 32>> node_key_map;
};


// Impl

//---------------- Helper macros begin ----------------//

#define DISCOVERYCLIENT_TEMPLATE typename DiscoveryClientDelegate, \
	template<typename> typename FiberTemplate

#define DISCOVERYCLIENT DiscoveryClient< \
	DiscoveryClientDelegate, \
	FiberTemplate \
>

//---------------- Helper macros end ----------------//


//---------------- Discovery protocol functions begin ----------------//


/*!
	function to get list of supported protocols on a client node
*/
template<DISCOVERYCLIENT_TEMPLATE>
void DISCOVERYCLIENT::send_DISCPROTO(
	FiberType& fiber,
	core::SocketAddress addr
) {
	fiber.o(*this).send(*this, DISCPROTO(), addr);
}


/*!
	\li Callback on receipt of disc proto
	\li Sends back the protocols supported on this node
*/
template<DISCOVERYCLIENT_TEMPLATE>
void DISCOVERYCLIENT::did_recv_DISCPROTO(
	FiberType& fiber,
	core::SocketAddress addr
) {
	SPDLOG_DEBUG("DISCPROTO <<< {}", addr.to_string());

	send_LISTPROTO(fiber, addr);
}


/*!
	sends the list of supported protocols on this node
*/
template<DISCOVERYCLIENT_TEMPLATE>
void DISCOVERYCLIENT::send_LISTPROTO(
	FiberType& fiber,
	core::SocketAddress addr
) {
	auto protocols = delegate->get_protocols();
	assert(protocols.size() < 100);

	fiber.o(*this).send(*this,
		LISTPROTO(protocols.size())
		.set_protocols(protocols.begin(), protocols.end()),
		addr
	);
}

template<DISCOVERYCLIENT_TEMPLATE>
void DISCOVERYCLIENT::did_recv_LISTPROTO(
	FiberType&,
	LISTPROTO &&packet,
	core::SocketAddress addr
) {
	SPDLOG_DEBUG("LISTPROTO <<< {}", addr.to_string());

	if(!packet.validate()) {
		return;
	}

	for(auto iter = packet.protocols_begin(); iter != packet.protocols_end(); ++iter) {
		auto [protocol, version, port] = *iter;
		core::SocketAddress peer_addr(addr);
		peer_addr.set_port(port);

		delegate->new_peer_protocol(peer_addr, node_key_map[addr].data(), protocol, version);
	}
}

/*!
	sends peer discovery message
*/
template<DISCOVERYCLIENT_TEMPLATE>
void DISCOVERYCLIENT::send_DISCPEER(
	FiberType& fiber,
	core::SocketAddress addr
) {
	fiber.o(*this).send(*this, DISCPEER(), addr);
}

/*!
	\li Callback on receipt of list peers
	\li Tries to connect to each of the peers via dial
*/
template<DISCOVERYCLIENT_TEMPLATE>
void DISCOVERYCLIENT::did_recv_LISTPEER(
	FiberType& fiber,
	LISTPEER &&packet,
	core::SocketAddress addr [[maybe_unused]]
) {
	SPDLOG_DEBUG("LISTPEER <<< {}", addr.to_string());

	if(!packet.validate()) {
		return;
	}

	for(auto iter = packet.peers_begin(); iter != packet.peers_end(); ++iter) {
		auto [peer_addr, key] = *iter;
		node_key_map[peer_addr] = key;

		fiber.dial(peer_addr, 0);
	}
}

/*!
	sends heartbeat message to refresh/create entry at the discovery server to keep the node discoverable
*/
template<DISCOVERYCLIENT_TEMPLATE>
void DISCOVERYCLIENT::send_HEARTBEAT(
	FiberType& fiber,
	core::SocketAddress addr
) {
	fiber.o(*this).send(*this, HEARTBEAT().set_key(static_pk), addr);
}

template<DISCOVERYCLIENT_TEMPLATE>
void DISCOVERYCLIENT::did_recv_DISCADDR(
	FiberType& fiber,
	core::SocketAddress addr
) {
	SPDLOG_DEBUG("DISCADDR <<< {}", addr.to_string());

	uint8_t name_size = name.size() > 255 ? 255 : name.size();

	BaseMessageType m(43 + 1 + name_size);
	auto p = m.payload_buffer();
	p.data()[0] = 6;
	std::memcpy(p.data()+1, address.c_str(), 42);
	p.data()[43] = name_size;
	std::memcpy(p.data()+44, name.c_str(), name_size);

	fiber.o(*this).send(*this, std::move(m), addr);
}

/*!
	callback to periodically send DISCPEER sending in search of new peers
*/
template<DISCOVERYCLIENT_TEMPLATE>
void DISCOVERYCLIENT::beacon_timer_cb() {
	for(auto& addr : discovery_addrs) {
		fiber.dial(addr, 1);
	}
}

/*!
	callback to periodically send HEARTBEAT to refresh the entry at beacon server
*/
template<DISCOVERYCLIENT_TEMPLATE>
void DISCOVERYCLIENT::heartbeat_timer_cb() {
	for(auto& addr : heartbeat_addrs) {
		fiber.dial(addr, 2);
	}
}

//---------------- Discovery protocol functions begin ----------------//


//---------------- Transport delegate functions begin ----------------//

template<DISCOVERYCLIENT_TEMPLATE>
int DISCOVERYCLIENT::did_dial(
	FiberType& fiber,
	core::SocketAddress addr,
	size_t type
) {
	if(type == 0) {
		// Normal peer
		send_DISCPROTO(fiber, addr);
	} else if(type == 1) {
		// Discovery peer
		send_DISCPEER(fiber, addr);
	} else if(type == 2) {
		// Heartbeat peer
		send_HEARTBEAT(fiber, addr);
	}

	return 0;
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
int DISCOVERYCLIENT::did_recv(
	FiberType& fiber,
	BaseMessageType &&packet,
	core::SocketAddress addr
) {
	if(!packet.validate()) {
		return -1;
	}

	auto type = packet.payload_buffer().read_uint8(0);
	if(type == std::nullopt) {
		return -1;
	}

	switch(type.value()) {
		// DISCPROTO
		case 0: did_recv_DISCPROTO(fiber, addr);
		break;
		// LISTPROTO
		case 1: did_recv_LISTPROTO(fiber, std::move(packet), addr);
		break;
		// DISCOVER
		case 2: SPDLOG_ERROR("Unexpected DISCPEER from {}", addr.to_string());
		break;
		// PEERLIST
		case 3: did_recv_LISTPEER(fiber, std::move(packet), addr);
		break;
		// HEARTBEAT
		case 4: SPDLOG_ERROR("Unexpected HEARTBEAT from {}", addr.to_string());
		break;
		// DISCADDR
		case 5: did_recv_DISCADDR(fiber, addr);
		break;
		// UNKNOWN
		default: SPDLOG_TRACE("UNKNOWN <<< {}", addr.to_string());
		break;
	}

	return 0;
}

template<DISCOVERYCLIENT_TEMPLATE>
int DISCOVERYCLIENT::did_send(
	FiberType&,
	core::Buffer &&packet
) {
	auto type = packet.read_uint8(1);
	if(type == std::nullopt || packet.read_uint8_unsafe(0) != 0) {
		return -1;
	}

	switch(type.value()) {
		// DISCPROTO
		case 0: SPDLOG_TRACE("DISCPROTO >>> {}", addr.to_string());
		break;
		// LISTPROTO
		case 1: SPDLOG_TRACE("LISTPROTO >>> {}", addr.to_string());
		break;
		// DISCPEER
		case 2: SPDLOG_TRACE("DISCPEER >>> {}", addr.to_string());
		break;
		// LISTPEER
		case 3: SPDLOG_TRACE("LISTPEER >>> {}", addr.to_string());
		break;
		// HEARTBEAT
		case 4: SPDLOG_TRACE("HEARTBEAT >>> {}", addr.to_string());
		break;
		// UNKNOWN
		default: SPDLOG_TRACE("UNKNOWN >>> {}", addr.to_string());
		break;
	}

	return 0;
}

//---------------- Transport delegate functions end ----------------//

template<DISCOVERYCLIENT_TEMPLATE>
template<typename ...Args>
DISCOVERYCLIENT::DiscoveryClient(
	core::SocketAddress const &addr,
	uint8_t const* static_sk,
	Args&&... args
) : fiber(std::forward_as_tuple(
	// ExtFabric which is Self&
	*this,
	// Internal fibers, simply forward
	std::forward<Args>(args)...
)), beacon_timer(this), heartbeat_timer(this) {
	fiber.bind(addr);
	fiber.listen();

	if(sodium_init() == -1) {
		throw;
	}

	std::memcpy(this->static_sk, static_sk, crypto_box_SECRETKEYBYTES);
	crypto_scalarmult_base(this->static_pk, this->static_sk);
}

template<DISCOVERYCLIENT_TEMPLATE>
DISCOVERYCLIENT::DiscoveryClient(
	core::SocketAddress const &addr,
	uint8_t const* static_sk
) : DiscoveryClient(addr, static_sk, std::make_tuple(), std::make_tuple()) {}

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
	discovery_addrs.push_back(beacon_addr);
	beacon_timer.template start<Self, &Self::beacon_timer_cb>(0, 60000);

	if(is_discoverable) {
		heartbeat_addrs.push_back(beacon_addr);
		heartbeat_timer.template start<Self, &Self::heartbeat_timer_cb>(0, 10000);
	}
}

template<DISCOVERYCLIENT_TEMPLATE>
void DISCOVERYCLIENT::start_discovery(
	std::vector<core::SocketAddress>&& discovery_addrs,
	std::vector<core::SocketAddress>&& heartbeat_addrs
) {
	this->discovery_addrs = std::move(discovery_addrs);
	this->heartbeat_addrs = std::move(heartbeat_addrs);
	beacon_timer.template start<Self, &Self::beacon_timer_cb>(0, 60000);
	heartbeat_timer.template start<Self, &Self::heartbeat_timer_cb>(0, 10000);
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
