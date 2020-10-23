#ifndef MARLIN_NEAR_NEARTRANSPORTFACTORY_HPP
#define MARLIN_NEAR_NEARTRANSPORTFACTORY_HPP

#include <fstream>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>

#include <marlin/asyncio/tcp/TcpTransportFactory.hpp>
#include <list>
#include "NearTransport.hpp"

namespace marlin {
namespace near {

template<typename ListenDelegate, typename TransportDelegate>
class NearTransportFactory {
private:
	asyncio::TcpTransportFactory<
		NearTransportFactory<ListenDelegate, TransportDelegate>,
		NearTransport<TransportDelegate>
	> f;
	ListenDelegate *delegate;
	std::list<NearTransport<TransportDelegate>> transportManager;

public:
	// Listen delegate
	bool should_accept(core::SocketAddress const &addr);
	void did_create_transport(
		asyncio::TcpTransport<NearTransport<TransportDelegate>> &transport
	);

	core::SocketAddress addr;

	NearTransportFactory();

	int bind(core::SocketAddress const &addr);
	int listen(ListenDelegate &delegate);
	int dial(core::SocketAddress const &addr, ListenDelegate &delegate);
};


// Impl

//---------------- Listen delegate functions begin ----------------//

template<typename ListenDelegate, typename TransportDelegate>
bool NearTransportFactory<ListenDelegate, TransportDelegate>::should_accept(
	core::SocketAddress const &addr
) {
	return delegate->should_accept(addr);
}

template<typename ListenDelegate, typename TransportDelegate>
void NearTransportFactory<ListenDelegate, TransportDelegate>::did_create_transport(
	asyncio::TcpTransport<NearTransport<TransportDelegate>> &transport
) {
	auto &near_transport = transportManager.emplace_back(
		transport.src_addr,
		transport.dst_addr,
		transport
	);
	return delegate->did_create_transport(near_transport);
}

//---------------- Listen delegate functions end ----------------//

template<typename ListenDelegate, typename TransportDelegate>
NearTransportFactory<ListenDelegate, TransportDelegate>::NearTransportFactory() {
	// NearCrypto c __attribute__((unused));
}

template<typename ListenDelegate, typename TransportDelegate>
int NearTransportFactory<ListenDelegate, TransportDelegate>::bind(
	core::SocketAddress const &addr
) {
	return f.bind(addr);
}

template<typename ListenDelegate, typename TransportDelegate>
int NearTransportFactory<ListenDelegate, TransportDelegate>::listen(
	ListenDelegate &delegate
) {
	this->delegate = &delegate;
	return f.listen(*this);
}

template<typename ListenDelegate, typename TransportDelegate>
int NearTransportFactory<ListenDelegate, TransportDelegate>::dial(
	core::SocketAddress const &addr,
	ListenDelegate &delegate
) {
	this->delegate = &delegate;
	return f.dial(addr, *this);
}

} // namespace near
} // namespace marlin

#endif // MARLIN_NEAR_NEARTRANSPORTFACTORY_HPP

