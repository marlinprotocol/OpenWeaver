/*! \file UdpTransport.hpp
	\brief Marlin virtual UDP Transport connection implementation

	Features:
	\li purely representation & virtual udp transport connection instance which is essentially a wrapper around libuv udp
	\li used to control UDP traffic to a particular destination
*/

#ifndef MARLIN_NET_UDPTRANSPORT_HPP
#define MARLIN_NET_UDPTRANSPORT_HPP

#include <marlin/net/Buffer.hpp>
#include <marlin/net/SocketAddress.hpp>
#include <marlin/net/core/TransportManager.hpp>
#include <uv.h>
#include <spdlog/spdlog.h>

#include <list>

namespace marlin {
namespace net {

//! Wrapper transport class around libuv udp functionality
template<typename DelegateType>
class UdpTransport {
private:
	uv_udp_t *socket = nullptr;
	TransportManager<UdpTransport<DelegateType>> &transport_manager;

	static void send_cb(
		uv_udp_send_t *req,
		int status
	);

	struct SendPayload {
		Buffer packet;
		UdpTransport<DelegateType> *transport;
	};

	std::list<uv_udp_send_t *> pending_req;
public:
	SocketAddress src_addr;
	SocketAddress dst_addr;

	DelegateType *delegate = nullptr;

	UdpTransport(
		SocketAddress const &src_addr,
		SocketAddress const &dst_addr,
		uv_udp_t *socket,
		TransportManager<UdpTransport<DelegateType>> &transport_manager
	);
	UdpTransport(UdpTransport const&) = delete;

	void setup(DelegateType *delegate);
	void did_recv_packet(Buffer &&packet);
	int send(Buffer &&packet);
	void close();
};


// Impl

template<typename DelegateType>
UdpTransport<DelegateType>::UdpTransport(
	SocketAddress const &_src_addr,
	SocketAddress const &_dst_addr,
	uv_udp_t *_socket,
	TransportManager<UdpTransport<DelegateType>> &transport_manager
) : socket(_socket), transport_manager(transport_manager),
	src_addr(_src_addr), dst_addr(_dst_addr), delegate(nullptr) {}


//! sets up the delegate when building an application or Higher Order Transport (Transport) over this transport
/*!
	\param delegate a DelegateType pointer to the application class instance which uses this transport
*/
template<typename DelegateType>
void UdpTransport<DelegateType>::setup(DelegateType *delegate) {
	this->delegate = delegate;
}

//! sends the incoming bytes to the application/HOT delegate
template<typename DelegateType>
void UdpTransport<DelegateType>::did_recv_packet(Buffer &&packet) {
	delegate->did_recv_packet(*this, std::move(packet));
}

template<typename DelegateType>
void UdpTransport<DelegateType>::send_cb(
	uv_udp_send_t *req,
	int status
) {
	auto *data = (SendPayload *)req->data;

	if(data->transport == nullptr) {
		delete data;
		delete req;
		return;
	}
	data->transport->pending_req.pop_front();

	if(status < 0) {
		SPDLOG_ERROR(
			"Net: Socket {}: Send callback error: {}",
			data->transport->dst_addr.to_string(),
			status
		);
	} else {
		data->transport->delegate->did_send_packet(
			*data->transport,
			std::move(data->packet)
		);
	}

	delete data;
	delete req;
}

//! called by higher level to send data
/*!
	\param packet Marlin::Buffer type of packet
	\return integer, 0 for success, failure otherwise
*/
template<typename DelegateType>
int UdpTransport<DelegateType>::send(Buffer &&packet) {
	uv_udp_send_t *req = new uv_udp_send_t();
	auto req_data = new SendPayload{std::move(packet), this};
	req->data = req_data;

	pending_req.push_back(req);

	auto buf = uv_buf_init((char*)req_data->packet.data(), req_data->packet.size());
	int res = uv_udp_send(
		req,
		socket,
		&buf,
		1,
		reinterpret_cast<const sockaddr *>(&dst_addr),
		UdpTransport<DelegateType>::send_cb
	);

	if (res < 0) {
		SPDLOG_ERROR(
			"Net: Socket {}: Send error: {}, To: {}",
			src_addr.to_string(),
			res,
			dst_addr.to_string()
		);
		return res;
	}

	return 0;
}

//! erases self entry from the transport manager which in turn destroys this instance. No other action required sinces its a virtual connection anyways
template<typename DelegateType>
void UdpTransport<DelegateType>::close() {
	delegate->did_close(*this);
	for (auto *req : pending_req) {
		auto *data = (SendPayload *)req->data;
		data->transport = nullptr;
	}
	transport_manager.erase(dst_addr);
}

} // namespace net
} // namespace marlin

#endif // MARLIN_NET_UDPTRANSPORT_HPP
