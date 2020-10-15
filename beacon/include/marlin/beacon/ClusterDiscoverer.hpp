#ifndef MARLIN_BEACON_CLUSTERDISCOVERER_HPP
#define MARLIN_BEACON_CLUSTERDISCOVERER_HPP

#include <marlin/core/transports/VersionedTransportFactory.hpp>
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
	typename ClusterDiscovererDelegate,
	template<typename, typename> class TransportFactory = asyncio::UdpTransportFactory,
	template<typename> class Transport = asyncio::UdpTransport
>
class ClusterDiscoverer {
private:
	using Self = ClusterDiscoverer<
		ClusterDiscovererDelegate,
		TransportFactory,
		Transport
	>;

	using BaseTransportFactory = core::VersionedTransportFactory<Self, Self, TransportFactory, Transport>;
	using BaseTransport = core::VersionedTransport<Self, Transport>;
	using BaseMessageType = typename BaseTransport::MessageType;
	using DISCPROTO = DISCPROTOWrapper<BaseMessageType>;
	using LISTPROTO = LISTPROTOWrapper<BaseMessageType>;
	using DISCPEER = DISCPEERWrapper<BaseMessageType>;
	using LISTPEER = LISTPEERWrapper<BaseMessageType>;
	using HEARTBEAT = HEARTBEATWrapper<BaseMessageType>;
	using DISCCLUSTER = DISCCLUSTERWrapper<BaseMessageType>;
	using LISTCLUSTER = LISTCLUSTERWrapper<BaseMessageType>;

	BaseTransportFactory f;

	// Discovery protocol
	void send_DISCPROTO(BaseTransport &transport);
	void did_recv_LISTPROTO(BaseTransport &transport, LISTPROTO &&packet);

	void send_DISCPEER(BaseTransport &transport);
	void did_recv_LISTPEER(BaseTransport &transport, LISTPEER &&packet);

	void send_DISCCLUSTER(BaseTransport &transport);
	void did_recv_LISTCLUSTER(BaseTransport &transport, LISTCLUSTER &&packet);

	std::vector<core::SocketAddress> discovery_addrs;

	void beacon_timer_cb();
	asyncio::Timer beacon_timer;

	void heartbeat_timer_cb();
	asyncio::Timer heartbeat_timer;

public:
	// Listen delegate
	bool should_accept(core::SocketAddress const &addr);
	void did_create_transport(BaseTransport &transport, size_t = 0);

	// Transport delegate
	void did_dial(BaseTransport &transport, size_t type = 0);
	void did_recv(BaseTransport &transport, BaseMessageType &&packet);
	void did_send(BaseTransport &transport, core::Buffer &&packet);

	template<typename ...Args>
	ClusterDiscoverer(
		core::SocketAddress const &addr,
		uint8_t const* static_sk,
		Args&&... args
	);
	~ClusterDiscoverer();

	ClusterDiscovererDelegate *delegate;

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
	std::unordered_map<core::SocketAddress, uint64_t> cluster_map;
	std::unordered_map<core::SocketAddress, std::pair<core::SocketAddress, uint64_t>> beacon_map;
};


// Impl

//---------------- Helper macros begin ----------------//

#define CLUSTERDISCOVERER_TEMPLATE typename ClusterDiscovererDelegate, \
	template<typename, typename> class TransportFactory, \
	template<typename> class Transport

#define CLUSTERDISCOVERER ClusterDiscoverer< \
	ClusterDiscovererDelegate, \
	TransportFactory, \
	Transport \
>

//---------------- Helper macros end ----------------//


//---------------- Discovery protocol functions begin ----------------//


/*!
	function to get list of supported protocols on a client node
*/
template<CLUSTERDISCOVERER_TEMPLATE>
void CLUSTERDISCOVERER::send_DISCPROTO(
	BaseTransport &transport
) {
	transport.send(DISCPROTO());
}

template<CLUSTERDISCOVERER_TEMPLATE>
void CLUSTERDISCOVERER::did_recv_LISTPROTO(
	BaseTransport &transport,
	LISTPROTO &&packet
) {
	SPDLOG_DEBUG("LISTPROTO <<< {}", transport.dst_addr.to_string());

	if(!packet.validate()) {
		return;
	}

	for(auto iter = packet.protocols_begin(); iter != packet.protocols_end(); ++iter) {
		auto [protocol, version, port] = *iter;
		core::SocketAddress peer_addr(transport.dst_addr);
		peer_addr.set_port(port);

		delegate->new_peer(beacon_map[transport.dst_addr].first, peer_addr, node_key_map[transport.dst_addr].data(), protocol, version);
	}
}

/*!
	sends peer discovery message
*/
template<CLUSTERDISCOVERER_TEMPLATE>
void CLUSTERDISCOVERER::send_DISCPEER(
	BaseTransport &transport
) {
	transport.send(DISCPEER());
}

/*!
	\li Callback on receipt of list peers
	\li Tries to connect to each of the peers via dial
*/
template<CLUSTERDISCOVERER_TEMPLATE>
void CLUSTERDISCOVERER::did_recv_LISTPEER(
	BaseTransport &transport [[maybe_unused]],
	LISTPEER &&packet
) {
	SPDLOG_DEBUG("LISTPEER <<< {}", transport.dst_addr.to_string());

	if(!packet.validate()) {
		return;
	}

	for(auto iter = packet.peers_begin(); iter != packet.peers_end(); ++iter) {
		auto [peer_addr, key] = *iter;
		node_key_map[peer_addr] = key;
        beacon_map[peer_addr] = std::make_pair(transport.dst_addr, asyncio::EventLoop::now());
		f.dial(peer_addr, *this, 0);
	}
}

/*!
	callback to periodically send DISCPEER sending in search of new peers
*/
template<CLUSTERDISCOVERER_TEMPLATE>
void CLUSTERDISCOVERER::beacon_timer_cb() {
    // Discover clusters
	for(auto& addr : discovery_addrs) {
		f.dial(addr, *this, 3);
	}

    // Prune clusters
    for(auto iter = cluster_map.begin(); iter != cluster_map.end();) {
        if(iter->second + 120000 < asyncio::EventLoop::now()) {
            // Stale cluster
            iter = cluster_map.erase(iter);
        } else {
			iter++;
		}
    }

    // Prune relay
    for(auto iter = beacon_map.begin(); iter != beacon_map.end();) {
        if(iter->second.second + 120000 < asyncio::EventLoop::now()) {
            // Stale cluster
            iter = beacon_map.erase(iter);
        } else {
			iter++;
		}
    }
}

template<CLUSTERDISCOVERER_TEMPLATE>
void CLUSTERDISCOVERER::send_DISCCLUSTER(
	BaseTransport &transport
) {
	transport.send(DISCCLUSTER());
}

template<CLUSTERDISCOVERER_TEMPLATE>
void CLUSTERDISCOVERER::did_recv_LISTCLUSTER(
	BaseTransport &transport [[maybe_unused]],
	LISTCLUSTER &&packet
) {
	SPDLOG_DEBUG("LISTCLUSTER <<< {}", transport.dst_addr.to_string());

	if(!packet.validate()) {
        SPDLOG_WARN("Validation failure");
		return;
	}

	for(auto iter = packet.clusters_begin(); iter != packet.clusters_end(); ++iter) {
		auto cluster_addr = *iter;
        SPDLOG_DEBUG("Cluster: {}", cluster_addr.to_string());
        cluster_map[cluster_addr] = asyncio::EventLoop::now();
		f.dial(cluster_addr, *this, 1);
	}
}

//---------------- Discovery protocol functions begin ----------------//


//---------------- Listen delegate functions begin ----------------//

template<CLUSTERDISCOVERER_TEMPLATE>
bool CLUSTERDISCOVERER::should_accept(
	core::SocketAddress const &
) {
	return false;
}

template<CLUSTERDISCOVERER_TEMPLATE>
void CLUSTERDISCOVERER::did_create_transport(
	BaseTransport &transport,
	size_t
) {
	transport.setup(this);
}

//---------------- Listen delegate functions end ----------------//


//---------------- Transport delegate functions begin ----------------//

template<CLUSTERDISCOVERER_TEMPLATE>
void CLUSTERDISCOVERER::did_dial(
	BaseTransport &transport,
	size_t type
) {
	if(type == 0) {
		// Normal peer
		send_DISCPROTO(transport);
	} else if(type == 1) {
		// Discovery peer
		send_DISCPEER(transport);
	} else if(type == 2) {
		// Heartbeat peer
	} else if(type == 3) {
        // Cluster discovery peer
        send_DISCCLUSTER(transport);
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
template<CLUSTERDISCOVERER_TEMPLATE>
void CLUSTERDISCOVERER::did_recv(
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
		case 0: SPDLOG_ERROR("Unexpected DISCPROTO from {}", transport.dst_addr.to_string());
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
		// DISCADDR
		case 5: SPDLOG_ERROR("Unexpected DISCADDR from {}", transport.dst_addr.to_string());
		break;
		// LISTCLUSTER
		case 8: did_recv_LISTCLUSTER(transport, std::move(packet));
		break;
		// UNKNOWN
		default: SPDLOG_TRACE("UNKNOWN <<< {}", transport.dst_addr.to_string());
		break;
	}
}

template<CLUSTERDISCOVERER_TEMPLATE>
void CLUSTERDISCOVERER::did_send(
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

template<CLUSTERDISCOVERER_TEMPLATE>
template<typename ...Args>
CLUSTERDISCOVERER::ClusterDiscoverer(
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

template<CLUSTERDISCOVERER_TEMPLATE>
CLUSTERDISCOVERER::~ClusterDiscoverer() {
	// Explicitly zero to protect memory
	sodium_memzero(static_sk, crypto_box_SECRETKEYBYTES);
}
/*!
	connects to the beacon server to start the peer discovery
*/
template<CLUSTERDISCOVERER_TEMPLATE>
void CLUSTERDISCOVERER::start_discovery(
	const core::SocketAddress &beacon_addr
) {
	discovery_addrs.push_back(beacon_addr);
	beacon_timer.template start<Self, &Self::beacon_timer_cb>(0, 60000);
}

template<CLUSTERDISCOVERER_TEMPLATE>
void CLUSTERDISCOVERER::start_discovery(
	std::vector<core::SocketAddress>&& discovery_addrs,
	std::vector<core::SocketAddress>&& heartbeat_addrs
) {
	this->discovery_addrs = std::move(discovery_addrs);
	this->heartbeat_addrs = std::move(heartbeat_addrs);
	beacon_timer.template start<Self, &Self::beacon_timer_cb>(0, 60000);
	heartbeat_timer.template start<Self, &Self::heartbeat_timer_cb>(0, 10000);
}

template<CLUSTERDISCOVERER_TEMPLATE>
void CLUSTERDISCOVERER::close() {
	beacon_timer.stop();
	heartbeat_timer.stop();
}


//---------------- Helper macros undef begin ----------------//

#undef CLUSTERDISCOVERER_TEMPLATE
#undef CLUSTERDISCOVERER

//---------------- Helper macros undef end ----------------//

} // namespace beacon
} // namespace marlin

#endif // MARLIN_BEACON_CLUSTERDISCOVERER_HPP
