/*! \file UdpTransport.hpp
	\brief Marlin TCP Transport connection implementation
*/


#ifndef MARLIN_NET_TCPTRANSPORT_HPP
#define MARLIN_NET_TCPTRANSPORT_HPP

#include <marlin/net/SocketAddress.hpp>
#include <marlin/net/core/TransportManager.hpp>
#include <uv.h>
#include <spdlog/spdlog.h>

namespace marlin {
namespace net {

//! Wrapper transport class around libuv tcp functionality
template<typename DelegateType>
class TcpTransport {
private:
	uv_tcp_t *socket;
	TransportManager<TcpTransport<DelegateType>> &transport_manager;

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
		Buffer bytes;
		TcpTransport<DelegateType> &transport;
	};
public:
	SocketAddress src_addr;
	SocketAddress dst_addr;

	DelegateType *delegate;

	TcpTransport(
		SocketAddress const &src_addr,
		SocketAddress const &dst_addr,
		uv_tcp_t *socket,
		TransportManager<TcpTransport<DelegateType>> &transport_manager
	);

	void setup(DelegateType *delegate);
	void did_recv_bytes(Buffer &&bytes);
	int send(Buffer &&bytes);
	void close();
};


// Impl

template<typename DelegateType>
TcpTransport<DelegateType>::TcpTransport(
	SocketAddress const &_src_addr,
	SocketAddress const &_dst_addr,
	uv_tcp_t *_socket,
	TransportManager<TcpTransport<DelegateType>> &transport_manager
) : socket(_socket), transport_manager(transport_manager),
	src_addr(_src_addr), dst_addr(_dst_addr) {}

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
			"Net: Socket {}: Recv callback error: {}",
			reinterpret_cast<SocketAddress const *>(&saddr)->to_string(),
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
		Buffer(buf->base, nread)
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
			"Net: Socket {}: Read start error: {}",
			src_addr.to_string(),
			res
		);
	}
}

//! sends the incoming bytes to the application/HOT delegate
template<typename DelegateType>
void TcpTransport<DelegateType>::did_recv_bytes(Buffer &&bytes) {
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
			"Net: Socket {}: Send callback error: {}",
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
	transport.transport_manager.erase(transport.dst_addr);
	delete handle;
}

//! called by higher level to send data
/*!
	\param bytes Marlin::Buffer type of packet
	\return integer, 0 for success, failure otherwise
*/
template<typename DelegateType>
int TcpTransport<DelegateType>::send(Buffer &&bytes) {
	auto *req = new uv_write_t();
	auto req_data = new SendPayload { std::move(bytes), *this };
	req->data = req_data;

	auto buf = uv_buf_init(req_data->bytes.data(), req_data->bytes.size());
	int res = uv_write(
		req,
		(uv_stream_t *)socket,
		&buf,
		1,
		send_cb
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

//! closes the underlying tcp socket. calls the close callback which erases self entry from the transport manager, which in turn destroys this instance
template<typename DelegateType>
void TcpTransport<DelegateType>::close() {
	uv_close((uv_handle_t *)socket, close_cb);
}

} // namespace net
} // namespace marlin

#endif // MARLIN_NET_TCPTRANSPORT_HPP
