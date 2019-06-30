#ifndef MARLIN_STREAM_STREAMTRANSPORTFACTORY_HPP
#define MARLIN_STREAM_STREAMTRANSPORTFACTORY_HPP

#include "StreamTransport.hpp"


namespace marlin {
namespace stream {

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
	std::unordered_map<
		net::SocketAddress,
		StreamTransport<
			TransportDelegate,
			DatagramTransport
		>
	> transport_map;

public:
	bool should_accept(net::SocketAddress const &addr);
	void did_create_transport(
		DatagramTransport<
			StreamTransport<
				TransportDelegate,
				DatagramTransport
			>
		> &transport
	);

	net::SocketAddress addr;

	int bind(net::SocketAddress const &addr);
	int listen(ListenDelegate &delegate);
	int dial(net::SocketAddress const &addr, ListenDelegate &delegate);
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
	> &transport
) {
	auto &stream_transport = transport_map.try_emplace(
		transport.dst_addr,
		transport.src_addr,
		transport.dst_addr,
		transport
	).first->second;
	delegate->did_create_transport(stream_transport);
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
>::bind(net::SocketAddress const &addr) {
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
>::dial(net::SocketAddress const &addr, ListenDelegate &delegate) {
	this->delegate = &delegate;
	return f.dial(addr, *this);
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
	auto iter = transport_map.find(addr);
	if(iter == transport_map.end()) {
		return nullptr;
	}

	return &iter->second;
}

} // namespace stream
} // namespace marlin

#endif // MARLIN_STREAM_STREAMTRANSPORTFACTORY_HPP
