#ifndef MARLIN_RLPX_RLPXTRANSPORTFACTORY_HPP
#define MARLIN_RLPX_RLPXTRANSPORTFACTORY_HPP

#include <fstream>

#include <cryptopp/eccrypto.h>
#include <cryptopp/files.h>
#include <cryptopp/filters.h>
#include <cryptopp/hmac.h>
#include <cryptopp/osrng.h>
#include <cryptopp/oids.h>
#include <cryptopp/sha.h>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>

#include <marlin/net/tcp/TcpTransportFactory.hpp>

#include "RlpxTransport.hpp"
#include "RlpxCrypto.hpp"

namespace marlin {
namespace rlpx {

template<typename ListenDelegate, typename TransportDelegate>
class RlpxTransportFactory {
private:
	net::TcpTransportFactory<
		RlpxTransportFactory<ListenDelegate, TransportDelegate>,
		RlpxTransport<TransportDelegate>
	> f;
	ListenDelegate *delegate;
	std::list<RlpxTransport<TransportDelegate>> transport_list;

public:
	// Listen delegate
	bool should_accept(net::SocketAddress const &addr);
	void did_create_transport(
		net::TcpTransport<RlpxTransport<TransportDelegate>> &transport
	);

	net::SocketAddress addr;

	RlpxTransportFactory();

	int bind(net::SocketAddress const &addr);
	int listen(ListenDelegate &delegate);
	int dial(net::SocketAddress const &addr, ListenDelegate &delegate);
};


// Impl

//---------------- Listen delegate functions begin ----------------//

template<typename ListenDelegate, typename TransportDelegate>
bool RlpxTransportFactory<ListenDelegate, TransportDelegate>::should_accept(
	net::SocketAddress const &addr
) {
	return delegate->should_accept(addr);
}

template<typename ListenDelegate, typename TransportDelegate>
void RlpxTransportFactory<ListenDelegate, TransportDelegate>::did_create_transport(
	net::TcpTransport<RlpxTransport<TransportDelegate>> &transport
) {
	auto &rlpx_transport = transport_list.emplace_back(
		transport.src_addr,
		transport.dst_addr,
		transport
	);
	return delegate->did_create_transport(rlpx_transport);
}

//---------------- Listen delegate functions end ----------------//

template<typename ListenDelegate, typename TransportDelegate>
RlpxTransportFactory<ListenDelegate, TransportDelegate>::RlpxTransportFactory() {
	RlpxCrypto c __attribute__((unused));
}

template<typename ListenDelegate, typename TransportDelegate>
int RlpxTransportFactory<ListenDelegate, TransportDelegate>::bind(
	net::SocketAddress const &addr
) {
	return f.bind(addr);
}

template<typename ListenDelegate, typename TransportDelegate>
int RlpxTransportFactory<ListenDelegate, TransportDelegate>::listen(
	ListenDelegate &delegate
) {
	this->delegate = &delegate;
	return f.listen(*this);
}

template<typename ListenDelegate, typename TransportDelegate>
int RlpxTransportFactory<ListenDelegate, TransportDelegate>::dial(
	net::SocketAddress const &addr,
	ListenDelegate &delegate
) {
	this->delegate = &delegate;
	return f.dial(addr, *this);
}

} // namespace rlpx
} // namespace marlin

#endif // MARLIN_RLPX_RLPXTRANSPORTFACTORY_HPP
