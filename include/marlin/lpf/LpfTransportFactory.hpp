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
	bool should_cut_through = false,
	int prefix_length = 8
>
class LpfTransportFactory {
private:
	using Self = LpfTransportFactory<
		ListenDelegate,
		TransportDelegate,
		StreamTransportFactory,
		StreamTransport,
		should_cut_through,
		prefix_length
	>;
	using SelfTransport = LpfTransport<
		TransportDelegate,
		StreamTransport,
		should_cut_through,
		prefix_length
	>;
	using BaseTransportFactory = StreamTransportFactory<
		Self,
		SelfTransport
	>;

	BaseTransportFactory f;

	ListenDelegate *delegate;
	net::TransportManager<SelfTransport> transport_manager;

public:
	bool should_accept(net::SocketAddress const &addr);
	void did_create_transport(
		StreamTransport<SelfTransport> &transport
	);

	net::SocketAddress addr;

	int bind(net::SocketAddress const &addr);
	int listen(ListenDelegate &delegate);
	int dial(net::SocketAddress const &addr, ListenDelegate &delegate, uint8_t const* keys = nullptr);
	SelfTransport *get_transport(
		net::SocketAddress const &addr
	);
};


// Impl

template<
	typename ListenDelegate,
	typename TransportDelegate,
	template<typename, typename> class StreamTransportFactory,
	template<typename> class StreamTransport,
	bool should_cut_through,
	int prefix_length
>
bool LpfTransportFactory<
	ListenDelegate,
	TransportDelegate,
	StreamTransportFactory,
	StreamTransport,
	should_cut_through,
	prefix_length
>::should_accept(net::SocketAddress const &addr) {
	return delegate->should_accept(addr);
}

template<
	typename ListenDelegate,
	typename TransportDelegate,
	template<typename, typename> class StreamTransportFactory,
	template<typename> class StreamTransport,
	bool should_cut_through,
	int prefix_length
>
void LpfTransportFactory<
	ListenDelegate,
	TransportDelegate,
	StreamTransportFactory,
	StreamTransport,
	should_cut_through,
	prefix_length
>::did_create_transport(
	StreamTransport<SelfTransport> &transport
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
	bool should_cut_through,
	int prefix_length
>
int LpfTransportFactory<
	ListenDelegate,
	TransportDelegate,
	StreamTransportFactory,
	StreamTransport,
	should_cut_through,
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
	bool should_cut_through,
	int prefix_length
>
int LpfTransportFactory<
	ListenDelegate,
	TransportDelegate,
	StreamTransportFactory,
	StreamTransport,
	should_cut_through,
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
	bool should_cut_through,
	int prefix_length
>
int LpfTransportFactory<
	ListenDelegate,
	TransportDelegate,
	StreamTransportFactory,
	StreamTransport,
	should_cut_through,
	prefix_length
>::dial(net::SocketAddress const &addr, ListenDelegate &delegate, uint8_t const* keys) {
	this->delegate = &delegate;

	if constexpr (IsTransportEncrypted<StreamTransport<TransportDelegate>>::value) {
		return f.dial(addr, *this, keys);
	} else {
		return f.dial(addr, *this);
	}
}

template<
	typename ListenDelegate,
	typename TransportDelegate,
	template<typename, typename> class StreamTransportFactory,
	template<typename> class StreamTransport,
	bool should_cut_through,
	int prefix_length
>
typename LpfTransportFactory<
	ListenDelegate,
	TransportDelegate,
	StreamTransportFactory,
	StreamTransport,
	should_cut_through,
	prefix_length
>::SelfTransport *
LpfTransportFactory<
	ListenDelegate,
	TransportDelegate,
	StreamTransportFactory,
	StreamTransport,
	should_cut_through,
	prefix_length
>::get_transport(
	net::SocketAddress const &addr
) {
	return transport_manager.get(addr);
}

} // namespace lpf
} // namespace marlin

#endif // MARLIN_LPF_LPFTRANSPORTFACTORY_HPP
