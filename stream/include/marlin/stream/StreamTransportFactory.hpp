#ifndef MARLIN_STREAM_STREAMTRANSPORTFACTORY_HPP
#define MARLIN_STREAM_STREAMTRANSPORTFACTORY_HPP

#include "StreamTransport.hpp"

namespace marlin {
namespace stream {

//! Factory class to create and manage StreamTranport instances
/*!
	Features:
	\li creates StreamTransport instances for every new peer
	\li exposes functions to bind to a socket, setting up a UDP listener and dialing to a peer
*/
template<
	typename ListenDelegate,
	typename TransportDelegate,
	template<typename, typename> class DatagramTransportFactory,
	template<typename> class DatagramTransport
>
class StreamTransportFactory {
private:
	typedef DatagramTransportFactory<
		StreamTransportFactory<
			ListenDelegate,
			TransportDelegate,
			DatagramTransportFactory,
			DatagramTransport
		>,
		StreamTransport<
			TransportDelegate,
			DatagramTransport
		>
	> BaseTransportFactory;

	BaseTransportFactory f;

	ListenDelegate *delegate;
	net::TransportManager<StreamTransport<TransportDelegate, DatagramTransport>> transport_manager;

public:
	bool should_accept(net::SocketAddress const &addr);
	void did_create_transport(
		DatagramTransport<
			StreamTransport<
				TransportDelegate,
				DatagramTransport
			>
		> &transport,
		uint8_t const* remote_static_pk = nullptr
	);

	net::SocketAddress addr;

	int bind(net::SocketAddress const &addr);
	int listen(ListenDelegate &delegate);
	int dial(net::SocketAddress const &addr, ListenDelegate &delegate, uint8_t const* key);
	StreamTransport<TransportDelegate, DatagramTransport> *get_transport(
		net::SocketAddress const &addr
	);
};


// Impl

template<
	typename ListenDelegate,
	typename TransportDelegate,
	template<typename, typename> class DatagramTransportFactory,
	template<typename> class DatagramTransport
>
bool StreamTransportFactory<
	ListenDelegate,
	TransportDelegate,
	DatagramTransportFactory,
	DatagramTransport
>::should_accept(net::SocketAddress const &addr) {
	return delegate->should_accept(addr);
}

/*!
	callback function after establishment of a new transport at lower level
*/
template<
	typename ListenDelegate,
	typename TransportDelegate,
	template<typename, typename> class DatagramTransportFactory,
	template<typename> class DatagramTransport
>
void StreamTransportFactory<
	ListenDelegate,
	TransportDelegate,
	DatagramTransportFactory,
	DatagramTransport
>::did_create_transport(
	DatagramTransport<
		StreamTransport<
			TransportDelegate,
			DatagramTransport
		>
	> &transport,
	uint8_t const* remote_static_pk
) {
	auto *stream_transport = transport_manager.get_or_create(
		transport.dst_addr,
		transport.src_addr,
		transport.dst_addr,
		transport,
		transport_manager,
		remote_static_pk
	).first;
	delegate->did_create_transport(*stream_transport);
}


//! calls bind() method of the underlying datagram transport factory
/*!
	/param addr address to bind to
	/return an integer 0 if successful, negative otherwise
*/
template<
	typename ListenDelegate,
	typename TransportDelegate,
	template<typename, typename> class DatagramTransportFactory,
	template<typename> class DatagramTransport
>
int StreamTransportFactory<
	ListenDelegate,
	TransportDelegate,
	DatagramTransportFactory,
	DatagramTransport
>::bind(net::SocketAddress const &addr) {
	this->addr = addr;
	return f.bind(addr);
}

//! calls listen() method of the underlying datagram transport factory to listen for requests on given address and sets up the given delegate as the listen delegate
/*!
	/param delegate sets up the given delegate as the listen delegate
	/return an integer 0 if successful, negative otherwise
*/
template<
	typename ListenDelegate,
	typename TransportDelegate,
	template<typename, typename> class DatagramTransportFactory,
	template<typename> class DatagramTransport
>
int StreamTransportFactory<
	ListenDelegate,
	TransportDelegate,
	DatagramTransportFactory,
	DatagramTransport
>::listen(ListenDelegate &delegate) {
	this->delegate = &delegate;
	return f.listen(*this);
}


//! calls dial() method of the underlying datagram transport factory and sets up the given delegate as the listen delegate
/*!
	/param addr socket address to dial/connect to
	/param delegate sets up the given delegate as the listen delegate
	/return an integer 0 if successful, negative otherwise
*/
template<
	typename ListenDelegate,
	typename TransportDelegate,
	template<typename, typename> class DatagramTransportFactory,
	template<typename> class DatagramTransport
>
int StreamTransportFactory<
	ListenDelegate,
	TransportDelegate,
	DatagramTransportFactory,
	DatagramTransport
>::dial(net::SocketAddress const &addr, ListenDelegate &delegate, uint8_t const* remote_static_pk) {
	this->delegate = &delegate;
	return f.dial(addr, *this, remote_static_pk);
}

template<
	typename ListenDelegate,
	typename TransportDelegate,
	template<typename, typename> class DatagramTransportFactory,
	template<typename> class DatagramTransport
>
StreamTransport<TransportDelegate, DatagramTransport> *
StreamTransportFactory<
	ListenDelegate,
	TransportDelegate,
	DatagramTransportFactory,
	DatagramTransport
>::get_transport(
	net::SocketAddress const &addr
) {
	return transport_manager.get(addr);
}

} // namespace stream
} // namespace marlin

#endif // MARLIN_STREAM_STREAMTRANSPORTFACTORY_HPP
