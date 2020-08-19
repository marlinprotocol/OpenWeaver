/*! \file UdpTransport.hpp
	\brief Marlin TCP Transport connection implementation
*/


#ifndef MARLIN_ASYNCIO_TCPTRANSPORT_HPP
#define MARLIN_ASYNCIO_TCPTRANSPORT_HPP

#include "marlin/core/SocketAddress.hpp"
#include "marlin/core/TransportManager.hpp"
#include <uv.h>
#include <spdlog/spdlog.h>

namespace marlin {
namespace asyncio {

//! Wrapper transport class around libuv tcp functionality
template<typename DelegateType>
class TcpTransport {
private:
	uv_tcp_t *socket;
	core::TransportManager<TcpTransport<DelegateType>> &transport_manager;

	static void naive_alloc_cb(
		uv_handle_t *,
		size_t suggested_size,
		uv_buf_t *buf
	);

	static void recv_cb(
		uv_stream_t *handle,
		ssize_t nread,
		uv_buf_t const *buf
	);

	static void send_cb(
		uv_write_t *req,
		int status
	);

	static void close_cb(uv_handle_t *handle);

	struct SendPayload {
		core::Buffer bytes;
		TcpTransport<DelegateType> &transport;
	};
public:
	core::SocketAddress src_addr;
	core::SocketAddress dst_addr;

	bool internal;

	DelegateType *delegate;

	TcpTransport(
		core::SocketAddress const &src_addr,
		core::SocketAddress const &dst_addr,
		uv_tcp_t *socket,
		core::TransportManager<TcpTransport<DelegateType>> &transport_manager
	);

	TcpTransport(TcpTransport const&) = delete;

	void setup(DelegateType *delegate);
	void did_recv_bytes(core::Buffer &&bytes);
	int send(core::Buffer &&bytes);
	uint16_t close_reason = 0;
	void close(uint16_t reason = 0);
};


// Impl

template<typename DelegateType>
TcpTransport<DelegateType>::TcpTransport(
	core::SocketAddress const &_src_addr,
	core::SocketAddress const &_dst_addr,
	uv_tcp_t *_socket,
	core::TransportManager<TcpTransport<DelegateType>> &transport_manager
) : socket(_socket), transport_manager(transport_manager),
	src_addr(_src_addr), dst_addr(_dst_addr) {
	if(
		core::CidrBlock::from_string("10.0.0.0/8").does_contain_address(dst_addr) ||
		core::CidrBlock::from_string("172.16.0.0/12").does_contain_address(dst_addr) ||
		core::CidrBlock::from_string("192.168.0.0/16").does_contain_address(dst_addr)
	) {
		internal = true;
	}
}

template<typename DelegateType>
void TcpTransport<DelegateType>::naive_alloc_cb(
	uv_handle_t *,
	size_t suggested_size,
	uv_buf_t *buf
) {
	buf->base = new char[suggested_size];
	buf->len = suggested_size;
}

//! callback function on receipt of any message on this TCP connection instance
template<typename DelegateType>
void TcpTransport<DelegateType>::recv_cb(
	uv_stream_t *handle,
	ssize_t nread,
	uv_buf_t const *buf
) {
	auto transport = (TcpTransport<DelegateType> *)handle->data;

	// EOF
	if(nread == -4095) {
		transport->close();
		delete[] buf->base;
		return;
	}

	// Error
	if(nread < 0) {
		sockaddr saddr;
		int len = sizeof(sockaddr);

		uv_tcp_getsockname((uv_tcp_t *)handle, &saddr, &len);

		SPDLOG_ERROR(
			"Asyncio: Socket {}: Recv callback error: {}",
			reinterpret_cast<core::SocketAddress const *>(&saddr)->to_string(),
			nread
		);

		delete[] buf->base;
		return;
	}

	if(nread == 0) {
		delete[] buf->base;
		return;
	}

	transport->did_recv_bytes(
		core::Buffer((uint8_t*)buf->base, nread)
	);
}

//! sets up the delegate when building an application or Higher Order Transport (Transport) over this transport
/*!
	\param delegate a DelegateType pointer to the application class instance which uses this transport
*/
template<typename DelegateType>
void TcpTransport<DelegateType>::setup(DelegateType *delegate) {
	this->delegate = delegate;

	socket->data = this;
	auto res = uv_read_start((uv_stream_t *)socket, naive_alloc_cb, recv_cb);

	if (res < 0) {
		SPDLOG_ERROR(
			"Asyncio: Socket {}: Read start error: {}",
			src_addr.to_string(),
			res
		);
	}
}

//! sends the incoming bytes to the application/HOT delegate
template<typename DelegateType>
void TcpTransport<DelegateType>::did_recv_bytes(core::Buffer &&bytes) {
	delegate->did_recv_bytes(*this, std::move(bytes));
}

template<typename DelegateType>
void TcpTransport<DelegateType>::send_cb(
	uv_write_t *req,
	int status
) {
	auto *data = (SendPayload *)req->data;

	if(status < 0) {
		SPDLOG_ERROR(
			"Asyncio: Socket {}: Send callback error: {}",
			data->transport.dst_addr.to_string(),
			status
		);
	} else {
		data->transport.delegate->did_send_bytes(
			data->transport,
			std::move(data->bytes)
		);
	}

	delete data;
	delete req;
}

template<typename DelegateType>
void TcpTransport<DelegateType>::close_cb(uv_handle_t *handle) {
	auto &transport = *(TcpTransport<DelegateType> *)handle->data;
	transport.delegate->did_close(transport, transport.close_reason);
	transport.transport_manager.erase(transport.dst_addr);
	delete handle;
}

//! called by higher level to send data
/*!
	\param bytes Marlin::core::Buffer type of packet
	\return integer, 0 for success, failure otherwise
*/
template<typename DelegateType>
int TcpTransport<DelegateType>::send(core::Buffer &&bytes) {
	auto *req = new uv_write_t();
	auto req_data = new SendPayload { std::move(bytes), *this };
	req->data = req_data;

	auto buf = uv_buf_init((char*)req_data->bytes.data(), req_data->bytes.size());
	int res = uv_write(
		req,
		(uv_stream_t *)socket,
		&buf,
		1,
		send_cb
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

//! closes the underlying tcp socket. calls the close callback which erases self entry from the transport manager, which in turn destroys this instance
template<typename DelegateType>
void TcpTransport<DelegateType>::close(uint16_t reason) {
	close_reason = reason;
	uv_close((uv_handle_t *)socket, close_cb);
}

} // namespace asyncio
} // namespace marlin

#endif // MARLIN_ASYNCIO_TCPTRANSPORT_HPP
