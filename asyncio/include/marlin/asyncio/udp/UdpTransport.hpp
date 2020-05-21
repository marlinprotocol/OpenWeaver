/*! \file UdpTransport.hpp
	\brief Marlin virtual UDP Transport connection implementation

	Features:
	\li purely representation & virtual udp transport connection instance which is essentially a wrapper around libuv udp
	\li used to control UDP traffic to a particular destination
*/

#ifndef MARLIN_ASYNCIO_UDPTRANSPORT_HPP
#define MARLIN_ASYNCIO_UDPTRANSPORT_HPP

#include <marlin/core/Buffer.hpp>
#include <marlin/core/messages/BaseMessage.hpp>
#include <marlin/core/SocketAddress.hpp>
#include <marlin/core/TransportManager.hpp>
#include <uv.h>
#include <spdlog/spdlog.h>

#include <list>

namespace marlin {
namespace asyncio {

//! Wrapper transport class around libuv udp functionality
template<typename DelegateType>
class UdpTransport {
private:
	uv_udp_t *socket = nullptr;
	core::TransportManager<UdpTransport<DelegateType>> &transport_manager;

	static void send_cb(
		uv_udp_send_t *req,
		int status
	);

	struct SendPayload {
		core::Buffer packet;
		UdpTransport<DelegateType> *transport;
	};

	std::list<uv_udp_send_t *> pending_req;

public:
	using MessageType = core::BaseMessage;

	core::SocketAddress src_addr;
	core::SocketAddress dst_addr;

	DelegateType *delegate = nullptr;

	UdpTransport(
		core::SocketAddress const &src_addr,
		core::SocketAddress const &dst_addr,
		uv_udp_t *socket,
		core::TransportManager<UdpTransport<DelegateType>> &transport_manager
	);
	UdpTransport(UdpTransport const&) = delete;

	void setup(DelegateType *delegate);
	void did_recv_packet(core::Buffer &&packet);
	int send(core::Buffer &&packet);
	int send(MessageType &&packet);
	void close();
};


// Impl

template<typename DelegateType>
UdpTransport<DelegateType>::UdpTransport(
	core::SocketAddress const &_src_addr,
	core::SocketAddress const &_dst_addr,
	uv_udp_t *_socket,
	core::TransportManager<UdpTransport<DelegateType>> &transport_manager
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
void UdpTransport<DelegateType>::did_recv_packet(core::Buffer &&packet) {
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
			"Asyncio: Socket {}: Send callback error: {}",
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
	\param packet Marlin::core::Buffer type of packet
	\return integer, 0 for success, failure otherwise
*/
template<typename DelegateType>
int UdpTransport<DelegateType>::send(core::Buffer &&packet) {
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
			"Asyncio: Socket {}: Send error: {}, To: {}",
			src_addr.to_string(),
			res,
			dst_addr.to_string()
		);
		return res;
	}

	return 0;
}

template<typename DelegateType>
int UdpTransport<DelegateType>::send(MessageType &&packet) {
	return send(std::move(packet).payload_buffer());
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

} // namespace asyncio
} // namespace marlin

#endif // MARLIN_ASYNCIO_UDPTRANSPORT_HPP
