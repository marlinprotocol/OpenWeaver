#ifndef MARLIN_NET_UDPTRANSPORTFACTORY_HPP
#define MARLIN_NET_UDPTRANSPORTFACTORY_HPP

#include <uv.h>
#include "Buffer.hpp"
#include "SocketAddress.hpp"
#include "UdpTransport.hpp"
#include <unordered_map>

#include <spdlog/spdlog.h>

namespace marlin {
namespace net {

template<typename ListenDelegate, typename TransportDelegate>
class UdpTransportFactory {
private:
	uv_udp_t *socket;

	std::unordered_map<
		SocketAddress,
		UdpTransport<TransportDelegate>
	> transport_map;

	static void recv_cb(
		uv_udp_t *handle,
		ssize_t nread,
		uv_buf_t const *buf,
		sockaddr const *addr,
		unsigned flags
	);
public:
	SocketAddress addr;

	UdpTransportFactory();
	~UdpTransportFactory();

	int bind(SocketAddress const &addr);
	int listen(ListenDelegate &delegate);
	int dial(SocketAddress const &addr, ListenDelegate &delegate);
};


// Impl

template<typename ListenDelegate, typename TransportDelegate>
struct RecvPayload {
	UdpTransportFactory<ListenDelegate, TransportDelegate> *factory;
	ListenDelegate *delegate;
};

template<typename ListenDelegate, typename TransportDelegate>
UdpTransportFactory<ListenDelegate, TransportDelegate>::UdpTransportFactory() {
	socket = new uv_udp_t();
}

template<typename ListenDelegate, typename TransportDelegate>
static inline void close_cb(uv_handle_t *handle) {
	delete static_cast<RecvPayload<ListenDelegate, TransportDelegate> *>(handle->data);
	delete handle;
}

template<typename ListenDelegate, typename TransportDelegate>
UdpTransportFactory<ListenDelegate, TransportDelegate>::~UdpTransportFactory() {
	uv_close((uv_handle_t *)socket, close_cb<ListenDelegate, TransportDelegate>);
}

template<typename ListenDelegate, typename TransportDelegate>
int UdpTransportFactory<ListenDelegate, TransportDelegate>::bind(SocketAddress const &addr) {
	this->addr = addr;

	uv_loop_t *loop = uv_default_loop();

	int res = uv_udp_init(loop, socket);
	if (res < 0) {
		SPDLOG_ERROR(
			"Net: Socket {}: Init error: {}",
			this->addr.to_string(),
			res
		);
		return res;
	}

	res = uv_udp_bind(socket, reinterpret_cast<sockaddr const *>(&this->addr), 0);
	if (res < 0) {
		SPDLOG_ERROR(
			"Net: Socket {}: Bind error: {}",
			this->addr.to_string(),
			res
		);
		return res;
	}

	return 0;
}

static inline void naive_alloc_cb(uv_handle_t *, size_t suggested_size, uv_buf_t *buf) {
	buf->base = new char[suggested_size];
	buf->len = suggested_size;
}

template<typename ListenDelegate, typename TransportDelegate>
void UdpTransportFactory<ListenDelegate, TransportDelegate>::recv_cb(
	uv_udp_t *handle,
	ssize_t nread,
	uv_buf_t const *buf,
	sockaddr const *_addr,
	unsigned
) {
	// Error
	if(nread < 0) {
		sockaddr saddr;
		int len = sizeof(sockaddr);

		uv_udp_getsockname(handle, &saddr, &len);

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

	auto &addr = *reinterpret_cast<SocketAddress const *>(_addr);
	auto payload = (RecvPayload<ListenDelegate, TransportDelegate> *)handle->data;

	auto &factory = *(payload->factory);
	auto &delegate = *static_cast<ListenDelegate *>(payload->delegate);

	auto iter = factory.transport_map.find(addr);
	if(iter == factory.transport_map.end()) {
		// Create new transport if permitted
		if(delegate.should_accept(addr)) {
			iter = factory.transport_map.try_emplace(
				addr,
				factory.addr,
				addr,
				factory.socket
			).first;
			delegate.did_create_transport(iter->second);
		} else {
			delete[] buf->base;
			return;
		}
	}

	auto &transport = iter->second;
	transport.did_recv_packet(
		Buffer(buf->base, nread)
	);
}

template<typename ListenDelegate, typename TransportDelegate>
int UdpTransportFactory<ListenDelegate, TransportDelegate>::listen(ListenDelegate &delegate) {
	delete static_cast<RecvPayload<ListenDelegate, TransportDelegate> *>(socket->data);
	socket->data = new RecvPayload<ListenDelegate, TransportDelegate>{this, &delegate};
	int res = uv_udp_recv_start(socket, naive_alloc_cb, UdpTransportFactory<ListenDelegate, TransportDelegate>::recv_cb);
	if (res < 0) {
		SPDLOG_ERROR(
			"Net: Socket {}: Start recv error: {}",
			this->addr.to_string(),
			res
		);
		return res;
	}

	return 0;
}

template<typename ListenDelegate, typename TransportDelegate>
int UdpTransportFactory<ListenDelegate, TransportDelegate>::dial(SocketAddress const &addr, ListenDelegate &delegate) {
	auto res = this->transport_map.try_emplace(
		addr,
		this->addr,
		addr,
		this->socket
	);

	auto &transport = res.first->second;

	if(res.second) {
		delegate.did_create_transport(transport);
	}

	transport.delegate->did_dial(transport);

	return 0;
}

} // namespace net
} // namespace marlin

#endif // MARLIN_NET_UDPTRANSPORTFACTORY_HPP
