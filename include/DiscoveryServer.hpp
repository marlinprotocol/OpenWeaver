#ifndef MARLIN_BEACON_BEACON_HPP
#define MARLIN_BEACON_BEACON_HPP

#include <marlin/net/udp/UdpTransportFactory.hpp>
#include <map>

namespace marlin {
namespace beacon {

template<typename DiscoveryServerDelegate>
class DiscoveryServer {
private:
	using BaseTransportFactory = net::UdpTransportFactory<
		DiscoveryServer<DiscoveryServerDelegate>,
		DiscoveryServer<DiscoveryServerDelegate>
	>;
	using BaseTransport = net::UdpTransport<
		DiscoveryServer<DiscoveryServerDelegate>
	>;

	BaseTransportFactory f;

	// Discovery protocol
	void did_recv_DISCPROTO(BaseTransport &transport);
	void send_LISTPROTO(BaseTransport &transport);

	void did_recv_DISCPEER(BaseTransport &transport);
	void send_LISTPEER(BaseTransport &transport);

	void did_recv_HEARTBEAT(BaseTransport &transport);

	static void heartbeat_timer_cb(uv_timer_t *handle);

	std::unordered_map<BaseTransport *, uint64_t> peers;
	uv_timer_t heartbeat_timer;
public:
	// Listen delegate
	bool should_accept(net::SocketAddress const &addr);
	void did_create_transport(BaseTransport &transport);

	// Transport delegate
	void did_dial(BaseTransport &transport);
	void did_recv_packet(BaseTransport &transport, net::Buffer &&packet);
	void did_send_packet(BaseTransport &transport, net::Buffer &&packet);

	DiscoveryServer(net::SocketAddress const &addr);

	DiscoveryServerDelegate *delegate;
};


// Impl

//---------------- Discovery protocol functions begin ----------------//

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
	char *message = new char[2] {0, 1};

	net::Buffer p(message, 2);
	transport.send(std::move(p));
}

template<typename DiscoveryServerDelegate>
void DiscoveryServer<DiscoveryServerDelegate>::did_recv_DISCPEER(
	BaseTransport &transport
) {
	SPDLOG_DEBUG("DISCPEER <<< {}", transport.dst_addr.to_string());

	send_LISTPEER(transport);
}

template<typename DiscoveryServerDelegate>
void DiscoveryServer<DiscoveryServerDelegate>::send_LISTPEER(
	BaseTransport &transport
) {
	char *message = new char[1100] {0, 3};

	size_t size = 2;

	// TODO - Handle overflow
	for(
		auto iter = peers.begin();
		iter != peers.end() && size < 1100;
		iter++, size += 8
	) {
		if(iter->first == &transport) continue;

		auto bytes = iter->first->dst_addr.serialize();
		std::memcpy(message+size, bytes.data(), 8);
	}

	net::Buffer p(message, size);
	transport.send(std::move(p));
}

template<typename DiscoveryServerDelegate>
void DiscoveryServer<DiscoveryServerDelegate>::did_recv_HEARTBEAT(
	BaseTransport &transport
) {
	SPDLOG_DEBUG("HEARTBEAT <<< {}", transport.dst_addr.to_string());

	peers[&transport] = uv_now(uv_default_loop());
}

template<typename DiscoveryServerDelegate>
void DiscoveryServer<DiscoveryServerDelegate>::heartbeat_timer_cb(uv_timer_t *handle) {
	auto &beacon = *(DiscoveryServer<DiscoveryServerDelegate> *)handle->data;

	auto now = uv_now(uv_default_loop());

	auto iter = beacon.peers.begin();
	while(iter != beacon.peers.end()) {
		// Remove stale peers if inactive for a minute
		if(now - iter->second > 60000) {
			iter = beacon.peers.erase(iter);
		} else {
			iter++;
		}
	}
}

//---------------- Discovery protocol functions begin ----------------//


//---------------- Listen delegate functions begin ----------------//

template<typename DiscoveryServerDelegate>
bool DiscoveryServer<DiscoveryServerDelegate>::should_accept(
	net::SocketAddress const &
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
	BaseTransport &
) {}

template<typename DiscoveryServerDelegate>
void DiscoveryServer<DiscoveryServerDelegate>::did_recv_packet(
	BaseTransport &transport,
	net::Buffer &&packet
) {
	switch(packet.read_uint8(1)) {
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
		case 4: did_recv_HEARTBEAT(transport);
		break;
		// UNKNOWN
		default: SPDLOG_TRACE("UNKNOWN <<< {}", transport.dst_addr.to_string());
		break;
	}
}

template<typename DiscoveryServerDelegate>
void DiscoveryServer<DiscoveryServerDelegate>::did_send_packet(
	BaseTransport &transport __attribute__((unused)),
	net::Buffer &&packet
) {
	switch(packet.read_uint8(1)) {
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

template<typename DiscoveryServerDelegate>
DiscoveryServer<DiscoveryServerDelegate>::DiscoveryServer(
	net::SocketAddress const &addr
) {
	f.bind(addr);
	f.listen(*this);

	uv_timer_init(uv_default_loop(), &heartbeat_timer);
	heartbeat_timer.data = this;

	uv_timer_start(
		&heartbeat_timer,
		&heartbeat_timer_cb,
		10000,
		10000
	);
}

} // namespace beacon
} // namespace marlin

#endif // MARLIN_BEACON_BEACON_HPP
