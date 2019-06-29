#ifndef MARLIN_NET_TCPTRANSPORTFACTORY_HPP
#define MARLIN_NET_TCPTRANSPORTFACTORY_HPP

#include <uv.h>
#include "Buffer.hpp"
#include "SocketAddress.hpp"
#include "TcpTransport.hpp"
#include <unordered_map>

#include <spdlog/spdlog.h>

namespace marlin {
namespace net {

template<typename ListenDelegate, typename TransportDelegate>
class TcpTransportFactory {
private:
	uv_tcp_t *socket;

	std::unordered_map<
		SocketAddress,
		TcpTransport<TransportDelegate>
	> transport_map;

	static void connection_cb(uv_stream_t *handle, int status);
	static void dial_cb(uv_connect_t *req, int status);
public:
	SocketAddress addr;

	TcpTransportFactory();
	~TcpTransportFactory();

	int bind(SocketAddress const &addr);
	int listen(ListenDelegate &delegate);
	int dial(SocketAddress const &addr, ListenDelegate &delegate);
	TcpTransport<TransportDelegate> *get_transport(
		SocketAddress const &addr
	);
};


// Impl

template<typename ListenDelegate, typename TransportDelegate>
struct ConnPayload {
	TcpTransportFactory<ListenDelegate, TransportDelegate> *factory;
	ListenDelegate *delegate;
};

template<typename ListenDelegate, typename TransportDelegate>
TcpTransportFactory<ListenDelegate, TransportDelegate>::
TcpTransportFactory() {
	socket = new uv_tcp_t();
}

template<typename ListenDelegate, typename TransportDelegate>
static inline void close_cb(uv_handle_t *handle) {
	delete static_cast<
		ConnPayload<ListenDelegate, TransportDelegate> *
	>(handle->data);
	delete handle;
}

template<typename ListenDelegate, typename TransportDelegate>
TcpTransportFactory<ListenDelegate, TransportDelegate>::
~TcpTransportFactory() {
	uv_close(
		(uv_handle_t *)socket,
		close_cb<ListenDelegate,
		TransportDelegate>
	);
}

template<typename ListenDelegate, typename TransportDelegate>
int
TcpTransportFactory<ListenDelegate, TransportDelegate>::
bind(SocketAddress const &addr) {
	this->addr = addr;

	uv_loop_t *loop = uv_default_loop();

	int res = uv_tcp_init(loop, socket);
	if (res < 0) {
		SPDLOG_ERROR(
			"Net: Socket {}: Init error: {}",
			this->addr.to_string(),
			res
		);
		return res;
	}

	res = uv_tcp_bind(
		socket,
		reinterpret_cast<sockaddr const *>(&this->addr),
		0
	);
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

static inline void client_close_cb(uv_handle_t *handle) {
	delete handle;
}

template<typename ListenDelegate, typename TransportDelegate>
void
TcpTransportFactory<ListenDelegate, TransportDelegate>::
connection_cb(uv_stream_t *handle, int status) {
	auto payload = static_cast<
		ConnPayload<ListenDelegate, TransportDelegate> *
	>(handle->data);

	auto &factory = *(payload->factory);
	auto &delegate = *static_cast<ListenDelegate *>(payload->delegate);

	if (status < 0) {
		SPDLOG_ERROR(
			"Net: Socket {}: Connection callback error: {}",
			factory.addr.to_string(),
			status
		);

		return;
	}

	auto *client = new uv_tcp_t();
	status = uv_tcp_init(uv_default_loop(), client);
	if (status < 0) {
		SPDLOG_ERROR(
			"Net: Socket {}: TCP init error: {}",
			factory.addr.to_string(),
			status
		);

		delete client;

		return;
	}

	status = uv_accept(handle, (uv_stream_t *)client);
	if (status < 0) {
		SPDLOG_ERROR(
			"Net: Socket {}: Connection accept error: {}",
			factory.addr.to_string(),
			status
		);

		delete client;

		return;
	}

	sockaddr saddr;
	int len = sizeof(sockaddr);

	status = uv_tcp_getpeername(client, &saddr, &len);
	if (status < 0) {
		SPDLOG_ERROR(
			"Net: Socket {}: Getpeername error: {}",
			factory.addr.to_string(),
			status
		);

		delete client;

		return;
	}

	auto &addr = *reinterpret_cast<SocketAddress const *>(&saddr);

	if(delegate.should_accept(addr)) {
		auto iter = factory.transport_map.try_emplace(
			addr,
			factory.addr,
			addr,
			client
		).first;
		delegate.did_create_transport(iter->second);
	} else {
		uv_close((uv_handle_t *)client, client_close_cb);
	}
}

template<typename ListenDelegate, typename TransportDelegate>
int
TcpTransportFactory<ListenDelegate, TransportDelegate>::
listen(ListenDelegate &delegate) {
	delete static_cast<
		ConnPayload<ListenDelegate, TransportDelegate> *
	>(socket->data);
	socket->data = new ConnPayload<ListenDelegate, TransportDelegate> {
		this,
		&delegate
	};

	int res = uv_listen((uv_stream_t *)socket, 100, connection_cb);
	if (res < 0) {
		SPDLOG_ERROR(
			"Net: Socket {}: Listen error: {}",
			this->addr.to_string(),
			res
		);
		return res;
	}

	return 0;
}

template<typename ListenDelegate, typename TransportDelegate>
struct DialPayload {
	TcpTransportFactory<ListenDelegate, TransportDelegate> *factory;
	ListenDelegate *delegate;
	SocketAddress addr;
};

template<typename ListenDelegate, typename TransportDelegate>
void
TcpTransportFactory<ListenDelegate, TransportDelegate>::
dial_cb(uv_connect_t *req, int status) {
	auto payload = (DialPayload<ListenDelegate, TransportDelegate> *)req->data;

	auto &factory = *(payload->factory);

	if (status < 0) {
		SPDLOG_ERROR(
			"Net: Socket {}: Connect error: {}",
			factory.addr.to_string(),
			status
		);

		delete (DialPayload<ListenDelegate, TransportDelegate> *)req->data;
		delete req;

		return;
	}

	auto &delegate = *static_cast<ListenDelegate *>(payload->delegate);
	auto &addr = payload->addr;

	auto res = factory.transport_map.try_emplace(
		addr,
		factory.addr,
		addr,
		factory.socket
	);
	auto &transport = res.first->second;

	if(res.second) {
		delegate.did_create_transport(transport);
	}

	transport.delegate->did_dial(transport);
}

template<typename ListenDelegate, typename TransportDelegate>
int
TcpTransportFactory<ListenDelegate, TransportDelegate>::
dial(SocketAddress const &addr, ListenDelegate &delegate) {
	auto *req = new uv_connect_t();
	req->data = new DialPayload<ListenDelegate, TransportDelegate> {
		this,
		&delegate,
		addr
	};

	uv_tcp_connect(
		req,
		socket,
		reinterpret_cast<sockaddr const *>(&addr),
		dial_cb
	);

	return 0;
}

template<typename ListenDelegate, typename TransportDelegate>
TcpTransport<TransportDelegate> *
TcpTransportFactory<ListenDelegate, TransportDelegate>::
get_transport(
	SocketAddress const &addr
) {
	auto iter = transport_map.find(addr);
	if(iter == transport_map.end()) {
		return nullptr;
	}

	return &iter->second;
}

} // namespace net
} // namespace marlin

#endif // MARLIN_NET_TCPTRANSPORTFACTORY_HPP
