/*! \file DiscoveryServer.hpp
	\brief Server side implementation of beacon functionality to register client nodes and provides info about other nodes (discovery)
*/

#ifndef MARLIN_BEACON_BEACON_HPP
#define MARLIN_BEACON_BEACON_HPP

#include <marlin/asyncio/core/Timer.hpp>
#include <marlin/asyncio/core/EventLoop.hpp>
#include <marlin/asyncio/udp/UdpFiber.hpp>
#include <marlin/core/fibers/VersioningFiber.hpp>
#include <marlin/core/fabric/Fabric.hpp>
#include <map>

#include <sodium.h>
#include <boost/iterator/filter_iterator.hpp>
#include <boost/iterator/transform_iterator.hpp>

#include <secp256k1_recovery.h>
#include <cryptopp/keccak.h>
#include <cryptopp/osrng.h>

#include <spdlog/fmt/bin_to_hex.h>

#include "Messages.hpp"
#include "PeerInfo.hpp"


namespace marlin {
namespace beacon {

//! Class implementing the server side node discovery functionality
/*!
	Features:
    \li Uses the custom marlin UDPTransport for message delivery
	\li HEARTBEAT - nodes which ping with a heartbeat are registered
	\li DISCPEER - nodes which ping with Discpeer are sent a list of peers
*/
template<
	typename DiscoveryServerDelegate,
	template<typename> typename FiberTemplate = core::FabricF<asyncio::UdpFiber, core::VersioningFiber>::type
>
class DiscoveryServer {
private:
	using Self = DiscoveryServer<
		DiscoveryServerDelegate,
		FiberTemplate
	>;
	using FiberType = FiberTemplate<Self&>;

	using BaseMessageType = typename FiberType::InnerMessageType;
	using DISCPROTO = DISCPROTOWrapper<BaseMessageType>;
	using LISTPROTO = LISTPROTOWrapper<BaseMessageType>;
	using DISCPEER = DISCPEERWrapper<BaseMessageType>;
	using LISTPEER = LISTPEERWrapper<BaseMessageType>;
	using HEARTBEAT = HEARTBEATWrapper<BaseMessageType>;
	using DISCCLUSTER = DISCCLUSTERWrapper<BaseMessageType>;
	using LISTCLUSTER = LISTCLUSTERWrapper<BaseMessageType>;
	using DISCCLUSTER2 = DISCCLUSTER2Wrapper<BaseMessageType>;
	using LISTCLUSTER2 = LISTCLUSTER2Wrapper<BaseMessageType>;

	FiberType fiber;
	FiberType h_fiber;

public:
	auto& i(auto&) {
		return *this;
	}

	auto& o(auto&) {
		return *this;
	}

	auto& is(auto& fiber) {
		return fiber;
	}

	auto& os(auto& fiber) {
		return fiber;
	}

private:
	// Discovery protocol
	void did_recv_DISCPROTO(FiberType& fiber, core::SocketAddress addr);
	void send_LISTPROTO(FiberType& fiber, core::SocketAddress addr);

	void did_recv_DISCPEER(FiberType& fiber, core::SocketAddress addr);
	void send_LISTPEER(FiberType& fiber, core::SocketAddress addr);

	void did_recv_HEARTBEAT(FiberType& fiber, HEARTBEAT &&bytes, core::SocketAddress addr);
	void did_recv_REG(FiberType& fiber, BaseMessageType&& packet, core::SocketAddress addr);

	void heartbeat_timer_cb();
	asyncio::Timer heartbeat_timer;

	void did_recv_DISCCLUSTER(FiberType& fiber, core::SocketAddress addr);
	void send_LISTCLUSTER(FiberType& fiber, core::SocketAddress addr);

	void did_recv_DISCCLUSTER2(FiberType& fiber, core::SocketAddress addr);
	void send_LISTCLUSTER2(FiberType& fiber, core::SocketAddress addr);

	std::unordered_map<core::SocketAddress, PeerInfo> peers;

	std::optional<core::SocketAddress> raddr = std::nullopt;

	secp256k1_context* ctx_signer = nullptr;
	secp256k1_context* ctx_verifier = nullptr;

	// Zero initialize explicitly
	uint8_t key[32] = {};

	bool is_key_empty() {
		uint8_t zero[32] = {};
		return std::memcmp(key, zero, 32) == 0;
	}
public:
	// Transport delegate
	int did_dial(FiberType& fiber, core::SocketAddress addr);
	int did_recv(FiberType& fiber, BaseMessageType &&packet, core::SocketAddress addr);
	int did_send(FiberType& fiber, core::Buffer &&packet);

private:
	template<typename ...Args>
	DiscoveryServer(
		core::SocketAddress const& baddr,
		core::SocketAddress const& haddr,
		std::optional<core::SocketAddress> raddr,
		std::string key,
		Args&&... args
	);

public:
	DiscoveryServer(
		core::SocketAddress const& baddr,
		core::SocketAddress const& haddr,
		std::optional<core::SocketAddress> raddr = std::nullopt,
		std::string key = ""
	);

	DiscoveryServerDelegate *delegate;
};


// Impl

//---------------- Helper macros begin ----------------//

#define DISCOVERYSERVER_TEMPLATE typename DiscoveryServerDelegate, \
	template<typename> typename FiberTemplate

#define DISCOVERYSERVER DiscoveryServer< \
	DiscoveryServerDelegate, \
	FiberTemplate \
>

//---------------- Helper macros end ----------------//


//---------------- Discovery protocol functions begin ----------------//


/*!
	\li Callback on receipt of disc proto
	\li Sends back the protocols supported on this node
*/
template<DISCOVERYSERVER_TEMPLATE>
void DISCOVERYSERVER::did_recv_DISCPROTO(
	FiberType& fiber,
	core::SocketAddress addr
) {
	SPDLOG_DEBUG("DISCPROTO <<< {}", addr.to_string());

	send_LISTPROTO(fiber, addr);
}

template<DISCOVERYSERVER_TEMPLATE>
void DISCOVERYSERVER::send_LISTPROTO(
	FiberType& fiber,
	core::SocketAddress addr
) {
	fiber.send(LISTPROTO(), addr);
}


/*!
	\li Callback on receipt of disc peer
	\li Sends back the list of peers
*/
template<DISCOVERYSERVER_TEMPLATE>
void DISCOVERYSERVER::did_recv_DISCPEER(
	FiberType& fiber,
	core::SocketAddress addr
) {
	SPDLOG_DEBUG("DISCPEER <<< {}", addr.to_string());

	send_LISTPEER(fiber, addr);
}


/*!
	sends the list of peers on this node
*/
template<DISCOVERYSERVER_TEMPLATE>
void DISCOVERYSERVER::send_LISTPEER(
	FiberType& fiber,
	core::SocketAddress addr
) {
	// Filter out the dst transport
	auto filter = [&](auto x) { return !(x.first == addr); };
	auto f_begin = boost::make_filter_iterator(filter, peers.begin(), peers.end());
	auto f_end = boost::make_filter_iterator(filter, peers.end(), peers.end());

	// Extract data into pair<addr, key> format
	auto transformation = [](auto x) { return std::make_pair(x.first, x.second.key); };
	auto t_begin = boost::make_transform_iterator(f_begin, transformation);
	auto t_end = boost::make_transform_iterator(f_end, transformation);

	while(t_begin != t_end) {
		fiber.send(LISTPEER(150).set_peers(t_begin, t_end), addr);
	}
}

/*!
	\li Callback on receipt of heartbeat from a client node
	\li Refreshes the entry of the node with current timestamp
*/
template<DISCOVERYSERVER_TEMPLATE>
void DISCOVERYSERVER::did_recv_HEARTBEAT(
	FiberType& fiber,
	HEARTBEAT &&bytes,
	core::SocketAddress addr
) {
	SPDLOG_INFO(
		"HEARTBEAT <<< {}, {:spn}",
		addr.to_string(),
		spdlog::to_hex(bytes.key(), bytes.key()+32)
	);

	if(!bytes.validate()) {
		return;
	}

	// Allow only heartbeat addr transports to add themselves to the registry
	if(&fiber != &this->h_fiber) {
		return;
	}

	peers[addr].last_seen = asyncio::EventLoop::now();
	peers[addr].key = bytes.key_array();
}

template<DISCOVERYSERVER_TEMPLATE>
void DISCOVERYSERVER::did_recv_REG(
	FiberType& fiber,
	BaseMessageType &&packet,
	core::SocketAddress addr
) {
	SPDLOG_INFO(
		"REG <<< {}",
		addr.to_string()
	);

	// Allow only heartbeat addr transports to add themselves to the registry
	if(&fiber != &this->h_fiber) {
		return;
	}

	SPDLOG_INFO(
		"REG <<< {}",
		addr.to_string()
	);

	auto payload = packet.payload_buffer();
	if(payload.size() > 73) {
		auto cur_time = (uint64_t)std::time(nullptr);
		auto p_time = payload.read_uint64_le_unsafe(1);

		if(cur_time - p_time > 60 && p_time - cur_time > 60) {
			// Too old or too in future
			SPDLOG_ERROR("REG: Time error");

			return;
		}

		uint8_t hash[32];
		CryptoPP::Keccak_256 hasher;
		// Hash message
		hasher.CalculateTruncatedDigest(hash, 32, payload.data()+1, 8);

		secp256k1_ecdsa_recoverable_signature sig;
		// Parse signature
		secp256k1_ecdsa_recoverable_signature_parse_compact(
			ctx_verifier,
			&sig,
			payload.data()+9,
			payload.data()[73]
		);

		// Verify signature
		secp256k1_pubkey pubkey;
		{
			auto res = secp256k1_ecdsa_recover(
				ctx_verifier,
				&pubkey,
				&sig,
				hash
			);

			if(res == 0) {
				// Recovery failed
				SPDLOG_ERROR("REG: Recovery failure: {}", spdlog::to_hex(payload.data(), payload.data() + payload.size()));

				return;
			}
		}

		uint8_t pubkeyser[65];
		size_t len = 65;
		secp256k1_ec_pubkey_serialize(
			ctx_verifier,
			pubkeyser,
			&len,
			&pubkey,
			SECP256K1_EC_UNCOMPRESSED
		);

		// Get address
		hasher.CalculateTruncatedDigest(hash, 32, pubkeyser+1, 64);
		// address is in hash[12..31]

		SPDLOG_DEBUG("Address: {:spn}", spdlog::to_hex(hash+12, hash+32));
		std::memcpy(peers[addr].address.data(), hash+12, 20);
	}

	peers[addr].last_seen = asyncio::EventLoop::now();

	constexpr bool has_new_reg = requires(
		DiscoveryServerDelegate d
	) {
		d.new_reg(core::SocketAddress(), PeerInfo());
	};

	if constexpr (has_new_reg) {
		delegate->new_reg(addr, peers[addr]);
	}
}


/*!
	callback to periodically cleanup the old peers which have been inactive for more than a minute (inactive = not received heartbeat)
*/
template<DISCOVERYSERVER_TEMPLATE>
void DISCOVERYSERVER::heartbeat_timer_cb() {
	auto now = asyncio::EventLoop::now();

	auto iter = peers.begin();
	while(iter != peers.end()) {
		// Remove stale peers if inactive for a minute
		if(now - iter->second.last_seen > 60000) {
			iter = peers.erase(iter);
		} else {
			iter++;
		}
	}

	if(raddr != std::nullopt) {
		auto time = (uint64_t)std::time(nullptr);
		SPDLOG_INFO("REG >>> {}", raddr->to_string());

		if(is_key_empty()) {
			BaseMessageType m(1);
			auto reg = m.payload_buffer();
			reg.data()[0] = 7;

			fiber.send(std::move(m), *raddr);
		} else {
			BaseMessageType m(74);
			auto reg = m.payload_buffer();
			reg.data()[0] = 7;
			reg.write_uint64_le_unsafe(1, time);

			uint8_t hash[32];
			CryptoPP::Keccak_256 hasher;
			// Hash message
			hasher.CalculateTruncatedDigest(hash, 32, reg.data()+1, 8);

			// Sign
			secp256k1_ecdsa_recoverable_signature sig;
			auto res = secp256k1_ecdsa_sign_recoverable(
				ctx_signer,
				&sig,
				hash,
				key,
				nullptr,
				nullptr
			);

			if(res == 0) {
				// Sign failed
				SPDLOG_ERROR("Beacon: Failed to send REG: signature failure");
				return;
			}

			// Output
			int recid;
			secp256k1_ecdsa_recoverable_signature_serialize_compact(
				ctx_signer,
				reg.data()+9,
				&recid,
				&sig
			);

			reg.data()[73] = (uint8_t)recid;

			fiber.send(std::move(m), *raddr);
		}
	}
}

template<DISCOVERYSERVER_TEMPLATE>
void DISCOVERYSERVER::did_recv_DISCCLUSTER(
	FiberType& fiber,
	core::SocketAddress addr
) {
	SPDLOG_DEBUG("DISCCLUSTER <<< {}", addr.to_string());

	send_LISTCLUSTER(fiber, addr);
}


template<DISCOVERYSERVER_TEMPLATE>
void DISCOVERYSERVER::send_LISTCLUSTER(
	FiberType& fiber,
	core::SocketAddress addr
) {
	// Filter out the dst transport
	auto filter = [&](auto x) { return !(x.first == addr); };
	auto f_begin = boost::make_filter_iterator(filter, peers.begin(), peers.end());
	auto f_end = boost::make_filter_iterator(filter, peers.end(), peers.end());

	// Extract data into addr format
	auto transformation = [](auto x) { return x.first; };
	auto t_begin = boost::make_transform_iterator(f_begin, transformation);
	auto t_end = boost::make_transform_iterator(f_end, transformation);

	while(t_begin != t_end) {
		fiber.send(LISTCLUSTER(150).set_clusters(t_begin, t_end), addr);
	}
}

template<DISCOVERYSERVER_TEMPLATE>
void DISCOVERYSERVER::did_recv_DISCCLUSTER2(
	FiberType& fiber,
	core::SocketAddress addr
) {
	SPDLOG_DEBUG("DISCCLUSTER <<< {}", addr.to_string());

	send_LISTCLUSTER2(fiber, addr);
}


template<DISCOVERYSERVER_TEMPLATE>
void DISCOVERYSERVER::send_LISTCLUSTER2(
	FiberType& fiber,
	core::SocketAddress addr
) {
	// Filter out the dst transport
	auto filter = [&](auto x) { return !(x.first == addr); };
	auto f_begin = boost::make_filter_iterator(filter, peers.begin(), peers.end());
	auto f_end = boost::make_filter_iterator(filter, peers.end(), peers.end());

	// Extract data into addr format
	auto transformation = [](auto x) { return std::make_tuple(x.first, x.second.address); };
	auto t_begin = boost::make_transform_iterator(f_begin, transformation);
	auto t_end = boost::make_transform_iterator(f_end, transformation);

	while(t_begin != t_end) {
		fiber.send(LISTCLUSTER2(150).set_clusters(t_begin, t_end), addr);
	}
}

//---------------- Discovery protocol functions end ----------------//


//---------------- Transport delegate functions begin ----------------//

template<DISCOVERYSERVER_TEMPLATE>
int DISCOVERYSERVER::did_dial(
	FiberType&,
	core::SocketAddress addr
) {
	raddr = addr;
	return 0;
}


//! receives the packet and processes them
/*!
	Determines the type of packet by reading the first byte and redirects the packet to appropriate function for further processing

	\li \b first-byte	:	\b type
	\li 0			:	DISCPROTO
	\li 1			:	ERROR - LISTPROTO, meant for client
	\li 2			:	DISCPEER
	\li 3			:	ERROR - LISTPEER, meant for client
	\li 4			:	HEARTBEAT
*/
template<DISCOVERYSERVER_TEMPLATE>
int DISCOVERYSERVER::did_recv(
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
		case 1: SPDLOG_ERROR("Unexpected LISTPROTO from {}", addr.to_string());
		break;
		// DISCOVER
		case 2: did_recv_DISCPEER(fiber, addr);
		break;
		// PEERLIST
		case 3: SPDLOG_ERROR("Unexpected LISTPEER from {}", addr.to_string());
		break;
		// HEARTBEAT
		case 4: did_recv_HEARTBEAT(fiber, std::move(packet), addr);
		break;
		// HEARTBEAT
		case 7: did_recv_REG(fiber, std::move(packet), addr);
		break;
		// DISCCLUSTER
		case 9: did_recv_DISCCLUSTER(fiber, addr);
		break;
		// LISTCLUSTER
		case 8: SPDLOG_ERROR("Unexpected LISTCLUSTER from {}", addr.to_string());
		break;
		// DISCCLUSTER2
		case 10: did_recv_DISCCLUSTER2(fiber, addr);
		break;
		// LISTCLUSTER2
		case 11: SPDLOG_ERROR("Unexpected LISTCLUSTER2 from {}", addr.to_string());
		break;
		// UNKNOWN
		default: SPDLOG_TRACE("UNKNOWN <<< {}", addr.to_string());
		break;
	}

	return 0;
}

template<DISCOVERYSERVER_TEMPLATE>
int DISCOVERYSERVER::did_send(
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
		// DISCCLUSTER
		case 9: SPDLOG_TRACE("DISCCLUSTER >>> {}", addr.to_string());
		break;
		// LISTCLUSTER
		case 8: SPDLOG_TRACE("LISTCLUSTER >>> {}", addr.to_string());
		break;
		// DISCCLUSTER2
		case 10: SPDLOG_TRACE("DISCCLUSTER2 >>> {}", addr.to_string());
		break;
		// LISTCLUSTER2
		case 11: SPDLOG_TRACE("LISTCLUSTER2 >>> {}", addr.to_string());
		break;
		// UNKNOWN
		default: SPDLOG_TRACE("UNKNOWN >>> {}", addr.to_string());
		break;
	}

	return 0;
}

//---------------- Transport delegate functions end ----------------//

template<DISCOVERYSERVER_TEMPLATE>
template<typename ...Args>
DISCOVERYSERVER::DiscoveryServer(
	core::SocketAddress const& baddr,
	core::SocketAddress const& haddr,
	std::optional<core::SocketAddress> raddr,
	std::string key,
	Args&&... args
) : fiber(std::forward_as_tuple(
	// ExtFabric which is Self&
	*this,
	// Internal fibers, simply forward
	std::forward<Args>(args)...
)), h_fiber(std::forward_as_tuple(
	// ExtFabric which is Self&
	*this,
	// Internal fibers, simply forward
	std::forward<Args>(args)...
)), heartbeat_timer(this) {
	fiber.bind(baddr);
	fiber.listen();

	h_fiber.bind(haddr);
	h_fiber.listen();

	heartbeat_timer.template start<Self, &Self::heartbeat_timer_cb>(0, 10000);

	ctx_signer = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
	ctx_verifier = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);
	if(key.size() == 0) {
		SPDLOG_INFO("Beacon: No identity key provided");
	} else if(key.size() == 32) {
		if(secp256k1_ec_seckey_verify(ctx_verifier, (uint8_t*)key.c_str()) != 1) {
			SPDLOG_ERROR("Beacon: failed to verify key", key.size());
			return;
		}
		std::memcpy(this->key, key.c_str(), 32);

		secp256k1_pubkey pubkey;
		auto res = secp256k1_ec_pubkey_create(
			ctx_signer,
			&pubkey,
			this->key
		);
		(void)res;

		uint8_t pubkeyser[65];
		size_t len = 65;
		secp256k1_ec_pubkey_serialize(
			ctx_signer,
			pubkeyser,
			&len,
			&pubkey,
			SECP256K1_EC_UNCOMPRESSED
		);

		// Get address
		uint8_t hash[32];
		CryptoPP::Keccak_256 hasher;
		hasher.CalculateTruncatedDigest(hash, 32, pubkeyser+1, 64);
		// address is in hash[12..31]

		SPDLOG_INFO("Beacon: Identity is 0x{:spn}", spdlog::to_hex(hash+12, hash+32));
	} else {
		SPDLOG_ERROR("Beacon: failed to load key: {}", key.size());
	}

	if(raddr.has_value()) {
		fiber.dial(*raddr);
	}
}

template<DISCOVERYSERVER_TEMPLATE>
DISCOVERYSERVER::DiscoveryServer(
	core::SocketAddress const& baddr,
	core::SocketAddress const& haddr,
	std::optional<core::SocketAddress> raddr,
	std::string key
) : DiscoveryServer(baddr, haddr, raddr, key, std::make_tuple(), std::make_tuple()) {}


//---------------- Helper macros undef begin ----------------//

#undef DISCOVERYSERVER_TEMPLATE
#undef DISCOVERYSERVER

//---------------- Helper macros undef end ----------------//

} // namespace beacon
} // namespace marlin

#endif // MARLIN_BEACON_BEACON_HPP
