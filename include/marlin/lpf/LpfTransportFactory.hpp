#ifndef MARLIN_LPF_LPFTRANSPORTFACTORY_HPP
#define MARLIN_LPF_LPFTRANSPORTFACTORY_HPP

#include "LpfTransport.hpp"


namespace marlin {
namespace lpf {

template<
	typename ListenDelegate,
	typename TransportDelegate,
	template<typename, typename> class StreamTransportFactory,
	template<typename> class StreamTransport,
	int prefix_length = 8
>
class LpfTransportFactory {
private:
	typedef StreamTransportFactory<
		LpfTransportFactory<
			ListenDelegate,
			TransportDelegate,
			StreamTransportFactory,
			StreamTransport
		>,
		LpfTransport<
			TransportDelegate,
			StreamTransport,
			prefix_length
		>
	> BaseTransportFactory;

	BaseTransportFactory f;

	ListenDelegate *delegate;
	net::TransportManager<LpfTransport<TransportDelegate, StreamTransport>> transport_manager;

public:
	bool should_accept(net::SocketAddress const &addr);
	void did_create_transport(
		StreamTransport<
			LpfTransport<
				TransportDelegate,
				StreamTransport,
				prefix_length
			>
		> &transport
	);

	net::SocketAddress addr;

	int bind(net::SocketAddress const &addr);
	int listen(ListenDelegate &delegate);
	int dial(net::SocketAddress const &addr, ListenDelegate &delegate);
	LpfTransport<TransportDelegate, StreamTransport> *get_transport(
		net::SocketAddress const &addr
	);
};


// Impl

template<
	typename ListenDelegate,
	typename TransportDelegate,
	template<typename, typename> class StreamTransportFactory,
	template<typename> class StreamTransport,
	int prefix_length
>
bool LpfTransportFactory<
	ListenDelegate,
	TransportDelegate,
	StreamTransportFactory,
	StreamTransport,
	prefix_length
>::should_accept(net::SocketAddress const &addr) {
	return delegate->should_accept(addr);
}

template<
	typename ListenDelegate,
	typename TransportDelegate,
	template<typename, typename> class StreamTransportFactory,
	template<typename> class StreamTransport,
	int prefix_length
>
void LpfTransportFactory<
	ListenDelegate,
	TransportDelegate,
	StreamTransportFactory,
	StreamTransport,
	prefix_length
>::did_create_transport(
	StreamTransport<
		LpfTransport<
			TransportDelegate,
			StreamTransport,
			prefix_length
		>
	> &transport
) {
	auto *lpf_transport = transport_manager.get_or_create(
		transport.dst_addr,
		transport.src_addr,
		transport.dst_addr,
		transport,
		transport_manager
	).first;
	delegate->did_create_transport(*lpf_transport);
}

template<
	typename ListenDelegate,
	typename TransportDelegate,
	template<typename, typename> class StreamTransportFactory,
	template<typename> class StreamTransport,
	int prefix_length
>
int LpfTransportFactory<
	ListenDelegate,
	TransportDelegate,
	StreamTransportFactory,
	StreamTransport,
	prefix_length
>::bind(net::SocketAddress const &addr) {
	this->addr = addr;
	return f.bind(addr);
}

template<
	typename ListenDelegate,
	typename TransportDelegate,
	template<typename, typename> class StreamTransportFactory,
	template<typename> class StreamTransport,
	int prefix_length
>
int LpfTransportFactory<
	ListenDelegate,
	TransportDelegate,
	StreamTransportFactory,
	StreamTransport,
	prefix_length
>::listen(ListenDelegate &delegate) {
	this->delegate = &delegate;
	return f.listen(*this);
}

template<
	typename ListenDelegate,
	typename TransportDelegate,
	template<typename, typename> class StreamTransportFactory,
	template<typename> class StreamTransport,
	int prefix_length
>
int LpfTransportFactory<
	ListenDelegate,
	TransportDelegate,
	StreamTransportFactory,
	StreamTransport,
	prefix_length
>::dial(net::SocketAddress const &addr, ListenDelegate &delegate) {
	this->delegate = &delegate;
	return f.dial(addr, *this);
}

template<
	typename ListenDelegate,
	typename TransportDelegate,
	template<typename, typename> class StreamTransportFactory,
	template<typename> class StreamTransport,
	int prefix_length
>
LpfTransport<TransportDelegate, StreamTransport> *
LpfTransportFactory<
	ListenDelegate,
	TransportDelegate,
	StreamTransportFactory,
	StreamTransport,
	prefix_length
>::get_transport(
	net::SocketAddress const &addr
) {
	return transport_manager.get(addr);
}

} // namespace lpf
} // namespace marlin

#endif // MARLIN_LPF_LPFTRANSPORTFACTORY_HPP
