#ifndef MARLIN_STREAM_STREAMTRANSPORTFACTORY_HPP
#define MARLIN_STREAM_STREAMTRANSPORTFACTORY_HPP

#include "StreamTransport.hpp"

namespace marlin {
namespace stream {

/// @brief Factory class to create and manage StreamTranport instances providing stream semantics.
/// Wraps around a base transport factory providing datagram semantics.
/// Exposes functions to bind to a socket, listening to incoming connections and dialing to a peer.
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
	core::TransportManager<StreamTransport<TransportDelegate, DatagramTransport>> transport_manager;

public:
	/// Constructor which forwards all arguments to the base transport's constructor
	template<typename ...Args>
	StreamTransportFactory(Args&&... args);

	/// Delegate callback from base transport on new incoming connection
	bool should_accept(core::SocketAddress const &addr);
	/// Delegate callback from base transport on creating a base transport
	void did_create_transport(
		DatagramTransport<
			StreamTransport<
				TransportDelegate,
				DatagramTransport
			>
		> &transport,
		uint8_t const* remote_static_pk = nullptr
	);

	/// Own address
	core::SocketAddress addr;

	/// Bind to the given interface and port
	int bind(core::SocketAddress const &addr);
	/// Listen for incoming connections and data
	int listen(ListenDelegate &delegate);
	/// Dial the given destination address
	int dial(core::SocketAddress const &addr, ListenDelegate &delegate, uint8_t const* key);
	/// Get the transport corresponding to the given destination address
	StreamTransport<TransportDelegate, DatagramTransport> *get_transport(
		core::SocketAddress const &addr
	);
};


// Impl

template<
	typename ListenDelegate,
	typename TransportDelegate,
	template<typename, typename> class DatagramTransportFactory,
	template<typename> class DatagramTransport
>
template<typename ...Args>
StreamTransportFactory<
	ListenDelegate,
	TransportDelegate,
	DatagramTransportFactory,
	DatagramTransport
>::StreamTransportFactory(Args&&... args) : f(std::forward<Args>(args)...) {}


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
>::should_accept(core::SocketAddress const &addr) {
	return delegate->should_accept(addr);
}

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
>::bind(core::SocketAddress const &addr) {
	this->addr = addr;
	return f.bind(addr);
}

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
>::dial(core::SocketAddress const &addr, ListenDelegate &delegate, uint8_t const* remote_static_pk) {
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
	core::SocketAddress const &addr
) {
	return transport_manager.get(addr);
}

} // namespace stream
} // namespace marlin

#endif // MARLIN_STREAM_STREAMTRANSPORTFACTORY_HPP
