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

#include <spdlog/fmt/bin_to_hex.h>

#include "Messages.hpp"


namespace marlin {
namespace beacon {

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
	void did_recv_REG(BaseTransport &transport);

	void heartbeat_timer_cb();
	asyncio::Timer heartbeat_timer;

	void did_recv_DISCCLUSTER(BaseTransport &transport);
	void send_LISTCLUSTER(BaseTransport &transport);

	std::unordered_map<core::SocketAddress, std::pair<uint64_t, std::array<uint8_t, 32>>> peers;

	BaseTransport* rt = nullptr;
public:
	// Listen delegate
	bool should_accept(core::SocketAddress const &addr);
	void did_create_transport(BaseTransport &transport);

	// Transport delegate
	void did_dial(BaseTransport &transport);
	void did_recv(BaseTransport &transport, BaseMessageType &&packet);
	void did_send(BaseTransport &transport, core::Buffer &&packet);

	DiscoveryServer(core::SocketAddress const& baddr, core::SocketAddress const& haddr, std::optional<core::SocketAddress> raddr = std::nullopt);

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
	auto transformation = [](auto x) { return std::make_pair(x.first, x.second.second); };
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

	peers[transport.dst_addr].first = asyncio::EventLoop::now();
	peers[transport.dst_addr].second = bytes.key_array();
}

template<typename DiscoveryServerDelegate>
void DiscoveryServer<DiscoveryServerDelegate>::did_recv_REG(
	BaseTransport &transport
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

	peers[transport.dst_addr].first = asyncio::EventLoop::now();
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
		if(now - iter->second.first > 60000) {
			iter = peers.erase(iter);
		} else {
			iter++;
		}
	}

	if(rt != nullptr) {
		SPDLOG_INFO("REG >>> {}", rt->dst_addr.to_string());
		core::Buffer reg(2);
		reg.data()[0] = 0;
		reg.data()[1] = 7;
		rt->send(std::move(reg));
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
	auto type = packet.payload_buffer().read_uint8(1);
	if(type == std::nullopt || packet.payload_buffer().read_uint8_unsafe(0) != 0) {
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
		case 7: did_recv_REG(transport);
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
	std::optional<core::SocketAddress> raddr
) : heartbeat_timer(this) {
	f.bind(baddr);
	f.listen(*this);

	hf.bind(haddr);
	hf.listen(*this);

	heartbeat_timer.template start<Self, &Self::heartbeat_timer_cb>(0, 10000);

	if(raddr.has_value()) {
		f.dial(*raddr, *this);
	}
}

} // namespace beacon
} // namespace marlin

#endif // MARLIN_BEACON_BEACON_HPP
