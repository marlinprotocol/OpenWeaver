#ifndef MARLIN_NET_TCPTRANSPORT_HPP
#define MARLIN_NET_TCPTRANSPORT_HPP

#include "SocketAddress.hpp"
#include "core/TransportManager.hpp"
#include <uv.h>
#include <spdlog/spdlog.h>

namespace marlin {
namespace net {

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

	struct SendPayload {
		Buffer &&packet;
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
	void did_recv_bytes(Buffer &&packet);
	int send(Buffer &&packet);
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

template<typename DelegateType>
void TcpTransport<DelegateType>::recv_cb(
	uv_stream_t *handle,
	ssize_t nread,
	uv_buf_t const *buf
) {
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

	auto transport = (TcpTransport<DelegateType> *)handle->data;
	transport->did_recv_bytes(
		Buffer(buf->base, nread)
	);
}

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

template<typename DelegateType>
void TcpTransport<DelegateType>::did_recv_bytes(Buffer &&packet) {
	delegate->did_recv_bytes(*this, std::move(packet));
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
			std::move(data->packet)
		);
	}

	delete data;
	delete req;
}

template<typename DelegateType>
int TcpTransport<DelegateType>::send(Buffer &&packet) {
	auto *req = new uv_write_t();
	auto req_data = new SendPayload { std::move(packet), *this };
	req->data = req_data;

	auto buf = uv_buf_init(req_data->packet.data(), req_data->packet.size());
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

} // namespace net
} // namespace marlin

#endif // MARLIN_NET_TCPTRANSPORT_HPP
