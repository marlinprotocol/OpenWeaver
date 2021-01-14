/*! \file DiscoveryServer.hpp
	\brief Server side implementation of beacon functionality to register client nodes and provides info about other nodes (discovery)
*/

#ifndef MARLIN_BEACON_BEACON_HPP
#define MARLIN_BEACON_BEACON_HPP

#include <marlin/core/transports/VersionedTransportFactory.hpp>
#include <marlin/asyncio/core/Timer.hpp>
#include <marlin/asyncio/core/EventLoop.hpp>
#include <marlin/asyncio/udp/UdpTransportFactory.hpp>
#include <map>

#include <sodium.h>
#include <boost/iterator/filter_iterator.hpp>
#include <boost/iterator/transform_iterator.hpp>

#include <secp256k1_recovery.h>
#include <cryptopp/keccak.h>
#include <cryptopp/osrng.h>

#include <spdlog/fmt/bin_to_hex.h>

#include "Messages.hpp"


namespace marlin {
namespace beacon {

struct PeerInfo {
	uint64_t last_seen;
	std::array<uint8_t, 32> key;
	std::array<uint8_t, 20> address;
};

//! Class implementing the server side node discovery functionality
/*!
	Features:
    \li Uses the custom marlin UDPTransport for message delivery
	\li HEARTBEAT - nodes which ping with a heartbeat are registered
	\li DISCPEER - nodes which ping with Discpeer are sent a list of peers
*/
template<typename DiscoveryServerDelegate>
class DiscoveryServer {
private:
	using Self = DiscoveryServer<DiscoveryServerDelegate>;

	using BaseTransportFactory = core::VersionedTransportFactory<Self, Self, asyncio::UdpTransportFactory, asyncio::UdpTransport>;
	using BaseTransport = core::VersionedTransport<Self, asyncio::UdpTransport>;
	using BaseMessageType = typename BaseTransport::MessageType;
	using DISCPROTO = DISCPROTOWrapper<BaseMessageType>;
	using LISTPROTO = LISTPROTOWrapper<BaseMessageType>;
	using DISCPEER = DISCPEERWrapper<BaseMessageType>;
	using LISTPEER = LISTPEERWrapper<BaseMessageType>;
	using HEARTBEAT = HEARTBEATWrapper<BaseMessageType>;
	using DISCCLUSTER = DISCCLUSTERWrapper<BaseMessageType>;
	using LISTCLUSTER = LISTCLUSTERWrapper<BaseMessageType>;

	BaseTransportFactory f;
	BaseTransportFactory hf;

	// Discovery protocol
	void did_recv_DISCPROTO(BaseTransport &transport);
	void send_LISTPROTO(BaseTransport &transport);

	void did_recv_DISCPEER(BaseTransport &transport);
	void send_LISTPEER(BaseTransport &transport);

	void did_recv_HEARTBEAT(BaseTransport &transport, HEARTBEAT &&bytes);
	void did_recv_REG(BaseTransport &transport, BaseMessageType&& packet);

	void heartbeat_timer_cb();
	asyncio::Timer heartbeat_timer;

	void did_recv_DISCCLUSTER(BaseTransport &transport);
	void send_LISTCLUSTER(BaseTransport &transport);

	std::unordered_map<core::SocketAddress, PeerInfo> peers;

	BaseTransport* rt = nullptr;

	secp256k1_context* ctx_signer = nullptr;
	secp256k1_context* ctx_verifier = nullptr;

	// Zero initialize explicitly
	uint8_t key[32] = {};

	bool is_key_empty() {
		uint8_t zero[32] = {};
		return std::memcmp(key, zero, 32) == 0;
	}
public:
	// Listen delegate
	bool should_accept(core::SocketAddress const &addr);
	void did_create_transport(BaseTransport &transport);

	// Transport delegate
	void did_dial(BaseTransport &transport);
	void did_recv(BaseTransport &transport, BaseMessageType &&packet);
	void did_send(BaseTransport &transport, core::Buffer &&packet);

	DiscoveryServer(
		core::SocketAddress const& baddr,
		core::SocketAddress const& haddr,
		std::optional<core::SocketAddress> raddr = std::nullopt,
		std::string key = ""
	);

	DiscoveryServerDelegate *delegate;
};


// Impl

//---------------- Discovery protocol functions begin ----------------//


/*!
	\li Callback on receipt of disc proto
	\li Sends back the protocols supported on this node
*/
template<typename DiscoveryServerDelegate>
void DiscoveryServer<DiscoveryServerDelegate>::did_recv_DISCPROTO(
	BaseTransport &transport
) {
	SPDLOG_DEBUG("DISCPROTO <<< {}", transport.dst_addr.to_string());

	send_LISTPROTO(transport);
}

template<typename DiscoveryServerDelegate>
void DiscoveryServer<DiscoveryServerDelegate>::send_LISTPROTO(
	BaseTransport &transport
) {
	transport.send(LISTPROTO());
}


/*!
	\li Callback on receipt of disc peer
	\li Sends back the list of peers
*/
template<typename DiscoveryServerDelegate>
void DiscoveryServer<DiscoveryServerDelegate>::did_recv_DISCPEER(
	BaseTransport &transport
) {
	SPDLOG_DEBUG("DISCPEER <<< {}", transport.dst_addr.to_string());

	send_LISTPEER(transport);
}


/*!
	sends the list of peers on this node
*/
template<typename DiscoveryServerDelegate>
void DiscoveryServer<DiscoveryServerDelegate>::send_LISTPEER(
	BaseTransport &transport
) {
	// Filter out the dst transport
	auto filter = [&](auto x) { return !(x.first == transport.dst_addr); };
	auto f_begin = boost::make_filter_iterator(filter, peers.begin(), peers.end());
	auto f_end = boost::make_filter_iterator(filter, peers.end(), peers.end());

	// Extract data into pair<addr, key> format
	auto transformation = [](auto x) { return std::make_pair(x.first, x.second.key); };
	auto t_begin = boost::make_transform_iterator(f_begin, transformation);
	auto t_end = boost::make_transform_iterator(f_end, transformation);

	while(t_begin != t_end) {
		transport.send(LISTPEER(150).set_peers(t_begin, t_end));
	}
}

/*!
	\li Callback on receipt of heartbeat from a client node
	\li Refreshes the entry of the node with current timestamp
*/
template<typename DiscoveryServerDelegate>
void DiscoveryServer<DiscoveryServerDelegate>::did_recv_HEARTBEAT(
	BaseTransport &transport,
	HEARTBEAT &&bytes
) {
	SPDLOG_INFO(
		"HEARTBEAT <<< {}, {:spn}",
		transport.dst_addr.to_string(),
		spdlog::to_hex(bytes.key(), bytes.key()+32)
	);

	if(!bytes.validate()) {
		return;
	}

	// Allow only heartbeat addr transports to add themselves to the registry
	if(!(transport.src_addr == hf.addr)) {
		return;
	}

	peers[transport.dst_addr].last_seen = asyncio::EventLoop::now();
	peers[transport.dst_addr].key = bytes.key_array();
}

template<typename DiscoveryServerDelegate>
void DiscoveryServer<DiscoveryServerDelegate>::did_recv_REG(
	BaseTransport &transport,
	BaseMessageType &&packet
) {
	SPDLOG_INFO(
		"REG <<< {}",
		transport.dst_addr.to_string()
	);

	// Allow only heartbeat addr transports to add themselves to the registry
	if(!(transport.src_addr == hf.addr)) {
		return;
	}

	SPDLOG_INFO(
		"REG <<< {}",
		transport.dst_addr.to_string()
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
		std::memcpy(peers[transport.dst_addr].address.data(), hash+12, 20);
	}

	peers[transport.dst_addr].last_seen = asyncio::EventLoop::now();

	constexpr bool has_new_reg = requires(
		DiscoveryServerDelegate d
	) {
		d.new_reg(core::SocketAddress(), PeerInfo());
	};

	if constexpr (has_new_reg) {
		delegate->new_reg(transport.dst_addr, peers[transport.dst_addr]);
	}
}


/*!
	callback to periodically cleanup the old peers which have been inactive for more than a minute (inactive = not received heartbeat)
*/
template<typename DiscoveryServerDelegate>
void DiscoveryServer<DiscoveryServerDelegate>::heartbeat_timer_cb() {
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

	if(rt != nullptr) {
		auto time = (uint64_t)std::time(nullptr);
		SPDLOG_INFO("REG >>> {}", rt->dst_addr.to_string());

		if(is_key_empty()) {
			BaseMessageType m(1);
			auto reg = m.payload_buffer();
			reg.data()[0] = 7;

			rt->send(std::move(m));
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

			rt->send(std::move(m));
		}
	}
}

template<typename DiscoveryServerDelegate>
void DiscoveryServer<DiscoveryServerDelegate>::did_recv_DISCCLUSTER(
	BaseTransport &transport
) {
	SPDLOG_DEBUG("DISCCLUSTER <<< {}", transport.dst_addr.to_string());

	send_LISTCLUSTER(transport);
}


template<typename DiscoveryServerDelegate>
void DiscoveryServer<DiscoveryServerDelegate>::send_LISTCLUSTER(
	BaseTransport &transport
) {
	// Filter out the dst transport
	auto filter = [&](auto x) { return !(x.first == transport.dst_addr); };
	auto f_begin = boost::make_filter_iterator(filter, peers.begin(), peers.end());
	auto f_end = boost::make_filter_iterator(filter, peers.end(), peers.end());

	// Extract data into addr format
	auto transformation = [](auto x) { return x.first; };
	auto t_begin = boost::make_transform_iterator(f_begin, transformation);
	auto t_end = boost::make_transform_iterator(f_end, transformation);

	while(t_begin != t_end) {
		transport.send(LISTCLUSTER(150).set_clusters(t_begin, t_end));
	}
}

//---------------- Discovery protocol functions begin ----------------//


//---------------- Listen delegate functions begin ----------------//

template<typename DiscoveryServerDelegate>
bool DiscoveryServer<DiscoveryServerDelegate>::should_accept(
	core::SocketAddress const &
) {
	return true;
}

template<typename DiscoveryServerDelegate>
void DiscoveryServer<DiscoveryServerDelegate>::did_create_transport(
	BaseTransport &transport
) {
	transport.setup(this);
}

//---------------- Listen delegate functions end ----------------//


//---------------- Transport delegate functions begin ----------------//

template<typename DiscoveryServerDelegate>
void DiscoveryServer<DiscoveryServerDelegate>::did_dial(
	BaseTransport &bt
) {
	rt = &bt;
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
template<typename DiscoveryServerDelegate>
void DiscoveryServer<DiscoveryServerDelegate>::did_recv(
	BaseTransport &transport,
	BaseMessageType &&packet
) {
	if(!packet.validate()) {
		return;
	}

	auto type = packet.payload_buffer().read_uint8(0);
	if(type == std::nullopt) {
		return;
	}

	switch(type.value()) {
		// DISCPROTO
		case 0: did_recv_DISCPROTO(transport);
		break;
		// LISTPROTO
		case 1: SPDLOG_ERROR("Unexpected LISTPROTO from {}", transport.dst_addr.to_string());
		break;
		// DISCOVER
		case 2: did_recv_DISCPEER(transport);
		break;
		// PEERLIST
		case 3: SPDLOG_ERROR("Unexpected LISTPEER from {}", transport.dst_addr.to_string());
		break;
		// HEARTBEAT
		case 4: did_recv_HEARTBEAT(transport, std::move(packet));
		break;
		// HEARTBEAT
		case 7: did_recv_REG(transport, std::move(packet));
		break;
		// DISCCLUSTER
		case 9: did_recv_DISCCLUSTER(transport);
		break;
		// LISTCLUSTER
		case 8: SPDLOG_ERROR("Unexpected LISTCLUSTER from {}", transport.dst_addr.to_string());
		break;
		// UNKNOWN
		default: SPDLOG_TRACE("UNKNOWN <<< {}", transport.dst_addr.to_string());
		break;
	}
}

template<typename DiscoveryServerDelegate>
void DiscoveryServer<DiscoveryServerDelegate>::did_send(
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
		// DISCCLUSTER
		case 9: SPDLOG_TRACE("DISCCLUSTER >>> {}", transport.dst_addr.to_string());
		break;
		// LISTCLUSTER
		case 8: SPDLOG_TRACE("LISTCLUSTER >>> {}", transport.dst_addr.to_string());
		break;
		// UNKNOWN
		default: SPDLOG_TRACE("UNKNOWN >>> {}", transport.dst_addr.to_string());
		break;
	}
}

//---------------- Transport delegate functions end ----------------//

template<typename DiscoveryServerDelegate>
DiscoveryServer<DiscoveryServerDelegate>::DiscoveryServer(
	core::SocketAddress const& baddr,
	core::SocketAddress const& haddr,
	std::optional<core::SocketAddress> raddr,
	std::string key
) : heartbeat_timer(this) {
	f.bind(baddr);
	f.listen(*this);

	hf.bind(haddr);
	hf.listen(*this);

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
		f.dial(*raddr, *this);
	}
}

} // namespace beacon
} // namespace marlin

#endif // MARLIN_BEACON_BEACON_HPP
