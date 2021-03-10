/*! \file UdpTransportFactory.hpp
	\brief Factory class to create and manage instances of marlin UDPTransport connections

	Uses a transport manager helper class to redirect the incoming UDP traffic to appropriate UDPTransport instance
*/

#ifndef MARLIN_ASYNCIO_UDPTRANSPORTFACTORY_HPP
#define MARLIN_ASYNCIO_UDPTRANSPORTFACTORY_HPP

#include <uv.h>
#include <marlin/uvpp/Udp.hpp>
#include <marlin/core/transports/TransportFactoryScaffold.hpp>
#include "marlin/core/Buffer.hpp"
#include "marlin/core/SocketAddress.hpp"
#include "UdpTransport.hpp"

#include <spdlog/spdlog.h>


namespace marlin {
namespace asyncio {


//! factory class to create instances of UDPTransport connection by either explicitly dialling or listening to incoming requests and messages
template<typename ListenDelegate, typename TransportDelegate>
class UdpTransportFactory : public core::TransportFactoryScaffold<
	UdpTransportFactory<ListenDelegate, TransportDelegate>,
	UdpTransport<TransportDelegate>,
	ListenDelegate,
	TransportDelegate,
	uvpp::UdpE*,
	uvpp::UdpE*
> {
public:
	using SelfType = UdpTransportFactory<ListenDelegate, TransportDelegate>;
	using SelfTransportType = UdpTransport<TransportDelegate>;
	using TransportFactoryScaffoldType = core::TransportFactoryScaffold<
		SelfType, SelfTransportType, ListenDelegate, TransportDelegate, uvpp::UdpE*, uvpp::UdpE*
	>;
private:
	using TransportFactoryScaffoldType::base_factory;
	using TransportFactoryScaffoldType::transport_manager;

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

public:
	using TransportFactoryScaffoldType::addr;

	UdpTransportFactory();
	~UdpTransportFactory();

	UdpTransportFactory(UdpTransportFactory const&) = delete;

	int bind(core::SocketAddress const &addr);
	int listen(ListenDelegate &delegate);

	template<typename... Args>
	int dial(core::SocketAddress const &addr, ListenDelegate &delegate, Args&&... args);

	using TransportFactoryScaffoldType::get_transport;
};


// Impl

template<typename ListenDelegate, typename TransportDelegate>
UdpTransportFactory<ListenDelegate, TransportDelegate>::
UdpTransportFactory() {
	base_factory = new uvpp::UdpE();
}

template<typename ListenDelegate, typename TransportDelegate>
void
UdpTransportFactory<ListenDelegate, TransportDelegate>::
close_cb(uv_handle_t *handle) {
	delete static_cast<
		RecvPayload *
	>(handle->data);
	delete (uvpp::UdpE*)(uv_udp_t*)handle;
}

//! Destructor, closes the listening socket
template<typename ListenDelegate, typename TransportDelegate>
UdpTransportFactory<ListenDelegate, TransportDelegate>::
~UdpTransportFactory() {
	uv_close(
		(uv_handle_t *)base_factory,
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
bind(core::SocketAddress const &addr) {
	this->addr = addr;

	uv_loop_t *loop = uv_default_loop();

	int res = uv_udp_init(loop, base_factory);
	if (res < 0) {
		SPDLOG_ERROR(
			"Asyncio: Socket {}: Init error: {}",
			this->addr.to_string(),
			res
		);
		return res;
	}

	res = uv_udp_bind(
		base_factory,
		reinterpret_cast<sockaddr const *>(&this->addr),
		0
	);
	if (res < 0) {
		SPDLOG_ERROR(
			"Asyncio: Socket {}: Bind error: {}",
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

	auto &addr = *reinterpret_cast<core::SocketAddress const *>(_addr);
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
				factory.base_factory,
				factory.transport_manager
			).first;
			delegate.did_create_transport(*transport);
		} else {
			delete[] buf->base;
			return;
		}
	}

	transport->did_recv(
		handle,
		core::Buffer((uint8_t*)buf->base, nread)
	);
}


//! starts listening for incoming messages on the socket address
template<typename ListenDelegate, typename TransportDelegate>
int
UdpTransportFactory<ListenDelegate, TransportDelegate>::
listen(ListenDelegate &delegate) {
	delete static_cast<
		RecvPayload *
	>(base_factory->data);
	base_factory->data = new RecvPayload {
		this,
		&delegate
	};
	int res = uv_udp_recv_start(
		base_factory,
		naive_alloc_cb,
		recv_cb
	);
	if (res < 0) {
		SPDLOG_ERROR(
			"Asyncio: Socket {}: Start recv error: {}",
			this->addr.to_string(),
			res
		);
		return res;
	}

	is_listening = true;

	return 0;
}

template<typename ListenDelegate, typename TransportDelegate>
template<typename... Args>
int
UdpTransportFactory<ListenDelegate, TransportDelegate>::
dial(core::SocketAddress const &addr, ListenDelegate &delegate, Args&&... args) {
	if(!is_listening) {
		auto status = listen(delegate);
		if(status < 0) {
			return status;
		}
	}

	auto [transport, res] = this->transport_manager.get_or_create(
		addr,
		this->addr,
		addr,
		this->base_factory,
		this->transport_manager
	);

	if(res) {
		delegate.did_create_transport(*transport);
	}

	transport->delegate->did_dial(*transport, std::forward<Args>(args)...);

	return res ? 1 : 0;
}

} // namespace asyncio
} // namespace marlin

#endif // MARLIN_ASYNCIO_UDPTRANSPORTFACTORY_HPP
