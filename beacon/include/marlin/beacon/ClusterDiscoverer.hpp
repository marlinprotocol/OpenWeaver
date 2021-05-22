#ifndef MARLIN_BEACON_CLUSTERDISCOVERER_HPP
#define MARLIN_BEACON_CLUSTERDISCOVERER_HPP

#include <marlin/asyncio/core/Timer.hpp>
#include <marlin/asyncio/udp/UdpFiber.hpp>
#include <marlin/core/fibers/VersioningFiber.hpp>
#include <marlin/core/fabric/Fabric.hpp>
#include <map>

#include <sodium.h>

#include "Messages.hpp"
#include "PeerInfo.hpp"


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
	template<typename> typename FiberTemplate = core::FabricF<asyncio::UdpFiber, core::VersioningFiber>::type
>
class ClusterDiscoverer {
private:
	using Self = ClusterDiscoverer<
		ClusterDiscovererDelegate,
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

	// Discovery protocol
	void send_DISCPROTO(FiberType& fiber, core::SocketAddress addr);
	void did_recv_LISTPROTO(FiberType& fiber, LISTPROTO&& packet, core::SocketAddress addr);

	void send_DISCPEER(FiberType& fiber, core::SocketAddress addr);
	void did_recv_LISTPEER(FiberType& fiber, LISTPEER&& packet, core::SocketAddress addr);

	void send_DISCCLUSTER(FiberType& fiber, core::SocketAddress addr);
	void did_recv_LISTCLUSTER(FiberType& fiber, LISTCLUSTER&& packet, core::SocketAddress addr);

	void send_DISCCLUSTER2(FiberType& fiber, core::SocketAddress addr);
	void did_recv_LISTCLUSTER2(FiberType& fiber, LISTCLUSTER2&& packet, core::SocketAddress addr);

	std::vector<core::SocketAddress> discovery_addrs;

	void beacon_timer_cb();
	asyncio::Timer beacon_timer;

	void heartbeat_timer_cb();
	asyncio::Timer heartbeat_timer;

public:
	// Transport delegate
	int did_dial(FiberType& fiber, core::SocketAddress addr, size_t type = 0);
	int did_recv(FiberType& fiber, BaseMessageType&& packet, core::SocketAddress addr);
	int did_send(FiberType& fiber, core::Buffer&& packet);

	ClusterDiscoverer(
		core::SocketAddress const& addr,
		uint8_t const* static_sk
	);
	template<typename ...Args>
	ClusterDiscoverer(
		core::SocketAddress const& addr,
		uint8_t const* static_sk,
		Args&&... args
	);
	~ClusterDiscoverer();

	ClusterDiscovererDelegate* delegate;

	void start_discovery(core::SocketAddress beacon_addr);
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
	std::unordered_map<core::SocketAddress, PeerInfo> cluster_map;
	std::unordered_map<core::SocketAddress, std::pair<core::SocketAddress, uint64_t>> beacon_map;
};


// Impl

//---------------- Helper macros begin ----------------//

#define CLUSTERDISCOVERER_TEMPLATE typename ClusterDiscovererDelegate, \
	template<typename> typename FiberTemplate

#define CLUSTERDISCOVERER ClusterDiscoverer< \
	ClusterDiscovererDelegate, \
	FiberTemplate \
>

//---------------- Helper macros end ----------------//


//---------------- Discovery protocol functions begin ----------------//


/*!
	function to get list of supported protocols on a client node
*/
template<CLUSTERDISCOVERER_TEMPLATE>
void CLUSTERDISCOVERER::send_DISCPROTO(
	FiberType& fiber,
	core::SocketAddress addr
) {
	fiber.send(DISCPROTO(), addr);
}

template<CLUSTERDISCOVERER_TEMPLATE>
void CLUSTERDISCOVERER::did_recv_LISTPROTO(
	FiberType&,
	LISTPROTO&& packet,
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

		delegate->new_peer_protocol(cluster_map[beacon_map[addr].first].address, peer_addr, node_key_map[addr].data(), protocol, version);
	}
}

/*!
	sends peer discovery message
*/
template<CLUSTERDISCOVERER_TEMPLATE>
void CLUSTERDISCOVERER::send_DISCPEER(
	FiberType& fiber,
	core::SocketAddress addr
) {
	fiber.send(DISCPEER(), addr);
}

/*!
	\li Callback on receipt of list peers
	\li Tries to connect to each of the peers via dial
*/
template<CLUSTERDISCOVERER_TEMPLATE>
void CLUSTERDISCOVERER::did_recv_LISTPEER(
	FiberType& fiber [[maybe_unused]],
	LISTPEER&& packet,
	core::SocketAddress addr
) {
	SPDLOG_DEBUG("LISTPEER <<< {}", addr.to_string());

	if(!packet.validate()) {
		return;
	}

	for(auto iter = packet.peers_begin(); iter != packet.peers_end(); ++iter) {
		auto [peer_addr, key] = *iter;
		node_key_map[peer_addr] = key;
		beacon_map[peer_addr] = std::make_pair(addr, asyncio::EventLoop::now());
		fiber.dial(peer_addr, 0);
	}
}

/*!
	callback to periodically send DISCPEER sending in search of new peers
*/
template<CLUSTERDISCOVERER_TEMPLATE>
void CLUSTERDISCOVERER::beacon_timer_cb() {
	// Discover clusters
	for(auto& addr : discovery_addrs) {
		fiber.dial(addr, 3);
	}

	// Prune clusters
	for(auto iter = cluster_map.begin(); iter != cluster_map.end();) {
		if(iter->second.last_seen + 120000 < asyncio::EventLoop::now()) {
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
	FiberType& fiber,
	core::SocketAddress addr
) {
	fiber.send(DISCCLUSTER(), addr);
}

template<CLUSTERDISCOVERER_TEMPLATE>
void CLUSTERDISCOVERER::did_recv_LISTCLUSTER(
	FiberType& fiber [[maybe_unused]],
	LISTCLUSTER&& packet,
	core::SocketAddress
) {
	SPDLOG_DEBUG("LISTCLUSTER <<< {}", addr.to_string());

	if(!packet.validate()) {
	SPDLOG_WARN("Validation failure");
		return;
	}

	for(auto iter = packet.clusters_begin(); iter != packet.clusters_end(); ++iter) {
		auto cluster_addr = *iter;
		SPDLOG_DEBUG("Cluster: {}", cluster_addr.to_string());
		cluster_map[cluster_addr].last_seen = asyncio::EventLoop::now();

		fiber.dial(cluster_addr, 1);
	}
}

template<CLUSTERDISCOVERER_TEMPLATE>
void CLUSTERDISCOVERER::send_DISCCLUSTER2(
	FiberType& fiber,
	core::SocketAddress addr
) {
	fiber.send(DISCCLUSTER2(), addr);
}

template<CLUSTERDISCOVERER_TEMPLATE>
void CLUSTERDISCOVERER::did_recv_LISTCLUSTER2(
	FiberType& fiber [[maybe_unused]],
	LISTCLUSTER2&& packet,
	core::SocketAddress
) {
	SPDLOG_DEBUG("LISTCLUSTER2 <<< {}", addr.to_string());

	if(!packet.validate()) {
	SPDLOG_WARN("Validation failure");
		return;
	}

	constexpr bool has_new_cluster = requires(
		ClusterDiscovererDelegate d
	) {
		d.new_cluster(core::SocketAddress(), std::array<uint8_t, 20>());
	};
	for(auto iter = packet.clusters_begin(); iter != packet.clusters_end(); ++iter) {
		auto cluster_addr = std::get<0>(*iter);
		auto cluster_client_key = std::get<1>(*iter);

		if constexpr (has_new_cluster) {
			delegate->new_cluster(cluster_addr, cluster_client_key);
		}

		cluster_map[cluster_addr].last_seen = asyncio::EventLoop::now();
		cluster_map[cluster_addr].address = cluster_client_key;
		fiber.dial(cluster_addr, 1);
	}
}

//---------------- Discovery protocol functions begin ----------------//


//---------------- Transport delegate functions begin ----------------//

template<CLUSTERDISCOVERER_TEMPLATE>
int CLUSTERDISCOVERER::did_dial(
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
	} else if(type == 3) {
		// Cluster discovery peer
		send_DISCCLUSTER2(fiber, addr);
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
template<CLUSTERDISCOVERER_TEMPLATE>
int CLUSTERDISCOVERER::did_recv(
	FiberType& fiber,
	BaseMessageType&& packet,
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
		case 0: SPDLOG_ERROR("Unexpected DISCPROTO from {}", addr.to_string());
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
		case 5: SPDLOG_ERROR("Unexpected DISCADDR from {}", addr.to_string());
		break;
		// LISTCLUSTER
		case 8: did_recv_LISTCLUSTER(fiber, std::move(packet), addr);
		break;
		// LISTCLUSTER2
		case 11: did_recv_LISTCLUSTER2(fiber, std::move(packet), addr);
		break;
		// UNKNOWN
		default: SPDLOG_TRACE("UNKNOWN <<< {}", addr.to_string());
		break;
	}

	return 0;
}

template<CLUSTERDISCOVERER_TEMPLATE>
int CLUSTERDISCOVERER::did_send(
	FiberType& fiber [[maybe_unused]],
	core::Buffer&& packet
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


template<CLUSTERDISCOVERER_TEMPLATE>
template<typename ...Args>
CLUSTERDISCOVERER::ClusterDiscoverer(
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

template<CLUSTERDISCOVERER_TEMPLATE>
CLUSTERDISCOVERER::ClusterDiscoverer(
	core::SocketAddress const &addr,
	uint8_t const* static_sk
) : ClusterDiscoverer(addr, static_sk, std::make_tuple(), std::make_tuple()) {}


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
	core::SocketAddress beacon_addr
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
