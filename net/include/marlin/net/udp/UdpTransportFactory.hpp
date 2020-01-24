/*! \file UdpTransportFactory.hpp
	\brief Factory class to create and manage instances of marlin UDPTransport connections

	Uses a transport manager helper class to redirect the incoming UDP traffic to appropriate UDPTransport instance
*/

#ifndef MARLIN_NET_UDPTRANSPORTFACTORY_HPP
#define MARLIN_NET_UDPTRANSPORTFACTORY_HPP

#include <uv.h>
#include <marlin/net/Buffer.hpp>
#include <marlin/net/SocketAddress.hpp>
#include "UdpTransport.hpp"

#include <spdlog/spdlog.h>

namespace marlin {
namespace net {

//! factory class to create instances of UDPTransport connection by either explicitly dialling or listening to incoming requests and messages
template<typename ListenDelegate, typename TransportDelegate>
class UdpTransportFactory {
private:
	uv_udp_t *socket = nullptr;
	TransportManager<UdpTransport<TransportDelegate>> transport_manager;

	static void naive_alloc_cb(
		uv_handle_t *,
		size_t suggested_size,
		uv_buf_t *buf
	);

	static void close_cb(uv_handle_t *handle);

	static void recv_cb(
		uv_udp_t *handle,
		ssize_t nread,
		uv_buf_t const *buf,
		sockaddr const *addr,
		unsigned flags
	);

	bool is_listening = false;

	struct RecvPayload {
		UdpTransportFactory<ListenDelegate, TransportDelegate> *factory;
		ListenDelegate *delegate;
	};

	std::pair<UdpTransport<TransportDelegate> *, int> dial_impl(SocketAddress const &addr, ListenDelegate &delegate);
public:
	SocketAddress addr;

	UdpTransportFactory();
	~UdpTransportFactory();

	int bind(SocketAddress const &addr);
	int listen(ListenDelegate &delegate);

	int dial(SocketAddress const &addr, ListenDelegate &delegate);
	template<typename MetadataType>
	int dial(SocketAddress const &addr, ListenDelegate &delegate, MetadataType* metadata);
	template<typename MetadataType>
	int dial(SocketAddress const &addr, ListenDelegate &delegate, MetadataType&& metadata);

	UdpTransport<TransportDelegate> *get_transport(
		SocketAddress const &addr
	);
};


// Impl

template<typename ListenDelegate, typename TransportDelegate>
UdpTransportFactory<ListenDelegate, TransportDelegate>::
UdpTransportFactory() {
	socket = new uv_udp_t();
}

template<typename ListenDelegate, typename TransportDelegate>
void
UdpTransportFactory<ListenDelegate, TransportDelegate>::
close_cb(uv_handle_t *handle) {
	delete static_cast<
		RecvPayload *
	>(handle->data);
	delete (uv_udp_t*)handle;
}

//! Destructor, closes the listening socket
template<typename ListenDelegate, typename TransportDelegate>
UdpTransportFactory<ListenDelegate, TransportDelegate>::
~UdpTransportFactory() {
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
UdpTransportFactory<ListenDelegate, TransportDelegate>::
bind(SocketAddress const &addr) {
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

	res = uv_udp_bind(
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
void UdpTransportFactory<ListenDelegate, TransportDelegate>::naive_alloc_cb(
	uv_handle_t *,
	size_t suggested_size,
	uv_buf_t *buf
) {
	buf->base = new char[suggested_size];
	buf->len = suggested_size;
}

//! callback on receiving a message on the socket
/*!
	\li redirects the read data to appropriate udp transport connection instance
	\li creates an instance if not already present
*/
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
	auto payload = (RecvPayload *)handle->data;

	auto &factory = *(payload->factory);
	auto &delegate = *static_cast<ListenDelegate *>(payload->delegate);

	auto *transport = factory.transport_manager.get(addr);
	if(transport == nullptr) {
		// Create new transport if permitted
		if(delegate.should_accept(addr)) {
			transport = factory.transport_manager.get_or_create(
				addr,
				factory.addr,
				addr,
				factory.socket,
				factory.transport_manager
			).first;
			delegate.did_create_transport(*transport);
		} else {
			delete[] buf->base;
			return;
		}
	}

	transport->did_recv_packet(
		Buffer(buf->base, nread)
	);
}


//! starts listening for incoming messages on the socket address
template<typename ListenDelegate, typename TransportDelegate>
int
UdpTransportFactory<ListenDelegate, TransportDelegate>::
listen(ListenDelegate &delegate) {
	delete static_cast<
		RecvPayload *
	>(socket->data);
	socket->data = new RecvPayload {
		this,
		&delegate
	};
	int res = uv_udp_recv_start(
		socket,
		naive_alloc_cb,
		recv_cb
	);
	if (res < 0) {
		SPDLOG_ERROR(
			"Net: Socket {}: Start recv error: {}",
			this->addr.to_string(),
			res
		);
		return res;
	}

	is_listening = true;

	return 0;
}

//! creates a UDP transport instance to the given address which can be used to send across messages by the delegate application or Higher order transport
/*
	/param addr address to dial to
	/delegate the listen delegate object
	/return 0 always, error handling done in dial_cb
*/
template<typename ListenDelegate, typename TransportDelegate>
std::pair<UdpTransport<TransportDelegate> *, int>
UdpTransportFactory<ListenDelegate, TransportDelegate>::
dial_impl(SocketAddress const &addr, ListenDelegate &delegate) {
	if(!is_listening) {
		auto status = listen(delegate);
		if(status < 0) {
			return {nullptr, status};
		}
	}

	auto [transport, res] = this->transport_manager.get_or_create(
		addr,
		this->addr,
		addr,
		this->socket,
		this->transport_manager
	);

	return {transport, res ? 1 : 0};
}

template<typename ListenDelegate, typename TransportDelegate>
int
UdpTransportFactory<ListenDelegate, TransportDelegate>::
dial(SocketAddress const &addr, ListenDelegate &delegate) {
	auto [transport, status] = dial_impl(addr, delegate);

	if(status < 0) {
		return status;
	} else if(status == 1) {
		delegate.did_create_transport(*transport);
	}

	transport->delegate->did_dial(*transport);

	return status;
}

template<typename ListenDelegate, typename TransportDelegate>
template<typename MetadataType>
int
UdpTransportFactory<ListenDelegate, TransportDelegate>::
dial(SocketAddress const &addr, ListenDelegate &delegate, MetadataType* metadata) {
	auto [transport, status] = dial_impl(addr, delegate);

	if(status < 0) {
		return status;
	} else if(status == 1) {
		delegate.did_create_transport(*transport, metadata);
	}

	transport->delegate->did_dial(*transport);

	return status;
}

template<typename ListenDelegate, typename TransportDelegate>
template<typename MetadataType>
int
UdpTransportFactory<ListenDelegate, TransportDelegate>::
dial(SocketAddress const &addr, ListenDelegate &delegate, MetadataType&& metadata) {
	auto [transport, status] = dial_impl(addr, delegate);

	if(status < 0) {
		return status;
	} else if(status == 1) {
		delegate.did_create_transport(*transport, std::move(metadata));
	}

	transport->delegate->did_dial(*transport);

	return status;
}

template<typename ListenDelegate, typename TransportDelegate>
UdpTransport<TransportDelegate> *
UdpTransportFactory<ListenDelegate, TransportDelegate>::
get_transport(
	SocketAddress const &addr
) {
	return transport_manager.get(addr);
}

} // namespace net
} // namespace marlin

#endif // MARLIN_NET_UDPTRANSPORTFACTORY_HPP
