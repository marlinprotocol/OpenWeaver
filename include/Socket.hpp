#ifndef MARLIN_NET_SOCKET_HPP
#define MARLIN_NET_SOCKET_HPP

#include "SocketAddress.hpp"
#include "Packet.hpp"
#include <map>
#include <spdlog/spdlog.h>

#include <functional>

namespace marlin {
namespace net {

/// UDP socket implementation wrapping libuv socket
class Socket
{
private:
	uv_udp_t _socket;

public:
	/// Address of socket
	SocketAddress addr;

	/// Default constructor
	Socket();

	/// Binds socket to given address
	int bind(const SocketAddress &_addr);

	/// Start receiving data from socket
	template<typename RecvDelegate>
	int start_receive(RecvDelegate &delegate);

	/// Send data through socket
	template<typename SendDelegate>
	int send(Packet &&p, const SocketAddress &addr, SendDelegate &delegate);
};


// Impl
// TODO - Proper error handling and return codes

static void naive_alloc_cb(uv_handle_t *, size_t suggested_size, uv_buf_t *buf) {
	buf->base = new char[suggested_size];
 	buf->len = suggested_size;
}

template<typename RecvDelegate>
static void recv_cb(
	uv_udp_t* handle,
	ssize_t nread,
	const uv_buf_t* buf,
	const struct sockaddr* addr,
	unsigned flags
) {
	// Error
	if(nread < 0) {
		delete[] buf->base;
		return;
	}

	// Empty data. TODO: Works for now. Investigate why it occurs.
	if(nread == 0) {
		delete[] buf->base;
		return;
	}

	RecvDelegate &delegate = *(RecvDelegate *)handle->data;
	delegate.did_receive_packet(
		std::move(Packet(buf->base, nread)),
		*reinterpret_cast<const SocketAddress *>(addr)
	);
}

template<typename RecvDelegate>
int Socket::start_receive(RecvDelegate &delegate) {
	_socket.data = (void *)(&delegate);
	int res = uv_udp_recv_start(&_socket, &naive_alloc_cb, &recv_cb<RecvDelegate>);
	if (res < 0) {
		spdlog::error("Fiber: Socket: Start recv error: {}", res);
		return res;
	}

	return 0;
}

template<typename SendDelegate>
struct SendReqData {
	Packet p;
	SendDelegate *delegate;

	SendReqData(Packet &&_p, SendDelegate *_delegate)
		: p(std::move(_p)), delegate(_delegate) {};
};

template<typename SendDelegate>
static void send_cb(uv_udp_send_t *req, int status) {
	spdlog::debug("Fiber: Socket: Send status: {}", status);

	SendReqData<SendDelegate> *data = (SendReqData<SendDelegate> *)req->data;

	data->delegate->did_send_packet(
		std::move(data->p),
		*reinterpret_cast<const SocketAddress *>(&req->addr)
	);

	delete data;
	delete req;
}

template<typename SendDelegate>
int Socket::send(Packet &&p, const SocketAddress &addr, SendDelegate &delegate) {
	uv_udp_send_t *req = new uv_udp_send_t();
	auto req_data = new SendReqData<SendDelegate>(std::move(p), &delegate);
	req->data = req_data;

	auto buf = uv_buf_init(req_data->p.data(), req_data->p.size());
	int res = uv_udp_send(req, &_socket, &buf, 1, reinterpret_cast<const sockaddr *>(&addr), &send_cb<SendDelegate>);

	if (res < 0) {
		spdlog::error("Fiber: Socket: Send error: {}", res);
		return res;
	}

	return 0;
}

} // namespace net
} // namespace marlin

#endif // MARLIN_NET_SOCKET_HPP
