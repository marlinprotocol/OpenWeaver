/*! \file TCPTransportFactory.hpp
	\brief Factory class to create and manage instances of marlin TCPTransport connections

	Note: listen() and dial() methods in the same class might create confusion
*/

#ifndef MARLIN_NET_TCPTRANSPORTFACTORY_HPP
#define MARLIN_NET_TCPTRANSPORTFACTORY_HPP

#include <uv.h>
#include <marlin/net/Buffer.hpp>
#include <marlin/net/SocketAddress.hpp>
#include "TcpTransport.hpp"

#include <spdlog/spdlog.h>

namespace marlin {
namespace net {

//! factory class to create instances of TCPTransport connections by either explicitly dialling or listening to incoming requests
/*!
	\li client mode: dial(), dial_cb() methods
	\li server mode: listen(), connection_cb() methods
*/
template<typename ListenDelegate, typename TransportDelegate>
class TcpTransportFactory {
private:
	uv_tcp_t *socket;
	TransportManager<TcpTransport<TransportDelegate>> transport_manager;

	static void close_cb(uv_handle_t *handle);
	static void client_close_cb(uv_handle_t *handle);

	static void connection_cb(uv_stream_t *handle, int status);
	static void dial_cb(uv_connect_t *req, int status);

	struct ConnPayload {
		TcpTransportFactory<ListenDelegate, TransportDelegate> *factory;
		ListenDelegate *delegate;
	};

	struct DialPayload {
		TcpTransportFactory<ListenDelegate, TransportDelegate> *factory;
		ListenDelegate *delegate;
		SocketAddress addr;
	};
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
TcpTransportFactory<ListenDelegate, TransportDelegate>::
TcpTransportFactory() {
	socket = new uv_tcp_t();
}

template<typename ListenDelegate, typename TransportDelegate>
void
TcpTransportFactory<ListenDelegate, TransportDelegate>::
close_cb(uv_handle_t *handle) {
	delete static_cast<
		ConnPayload *
	>(handle->data);
	delete handle;
}

//! Destructor, closes socket
template<typename ListenDelegate, typename TransportDelegate>
TcpTransportFactory<ListenDelegate, TransportDelegate>::
~TcpTransportFactory() {
	uv_close(
		(uv_handle_t *)socket,
		close_cb
	);
}

//! binds the socket to given arg address
/*!
	/param addr address to bind the socket to
	/return an integer 0 if successful, negative otherwise
*/
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

template<typename ListenDelegate, typename TransportDelegate>
void
TcpTransportFactory<ListenDelegate, TransportDelegate>::
client_close_cb(uv_handle_t *handle) {
	delete handle;
}

//! callback on receipt of new connection request
/*!
	\li accepts the incoming tcp connection
	\li creates a TCPTransport instance to handle any further communication on this TCP connection
*/
template<typename ListenDelegate, typename TransportDelegate>
void
TcpTransportFactory<ListenDelegate, TransportDelegate>::
connection_cb(uv_stream_t *handle, int status) {
	auto payload = static_cast<
		ConnPayload *
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
		auto *transport = factory.transport_manager.get_or_create(
			addr,
			factory.addr,
			addr,
			client,
			factory.transport_manager
		).first;
		delegate.did_create_transport(*transport);
	} else {
		uv_close((uv_handle_t *)client, client_close_cb);
	}
}

//! starts listening for incoming connection requests on the socket address
template<typename ListenDelegate, typename TransportDelegate>
int
TcpTransportFactory<ListenDelegate, TransportDelegate>::
listen(ListenDelegate &delegate) {
	delete static_cast<
		ConnPayload *
	>(socket->data);
	socket->data = new ConnPayload {
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

//! client mode callback function called on successful dial to the server instance
template<typename ListenDelegate, typename TransportDelegate>
void
TcpTransportFactory<ListenDelegate, TransportDelegate>::
dial_cb(uv_connect_t *req, int status) {
	auto payload = (DialPayload *)req->data;

	auto &factory = *(payload->factory);

	if (status < 0) {
		SPDLOG_ERROR(
			"Net: Socket {}: Connect error: {}",
			factory.addr.to_string(),
			status
		);

		delete (DialPayload *)req->data;
		delete req;

		return;
	}

	auto &delegate = *static_cast<ListenDelegate *>(payload->delegate);
	auto &addr = payload->addr;

	auto [transport, res] = factory.transport_manager.get_or_create(
		addr,
		factory.addr,
		addr,
		factory.socket,
		factory.transport_manager
	);

	if(res) {
		delegate.did_create_transport(*transport);
	}

	transport->delegate->did_dial(*transport);
}


//! client mode function that tries to establish a TCP connection to the given server socket address
/*
	/param addr address to dial to
	/delegate the listen delegate object
	/return 0 always, error handling done in dial_cb
*/
template<typename ListenDelegate, typename TransportDelegate>
int
TcpTransportFactory<ListenDelegate, TransportDelegate>::
dial(SocketAddress const &addr, ListenDelegate &delegate) {
	auto *req = new uv_connect_t();
	req->data = new DialPayload {
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
	return transport_manager.get(addr);
}

} // namespace net
} // namespace marlin

#endif // MARLIN_NET_TCPTRANSPORTFACTORY_HPP
