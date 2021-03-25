#ifndef MARLIN_STREAM_STREAMTRANSPORTFACTORY_HPP
#define MARLIN_STREAM_STREAMTRANSPORTFACTORY_HPP

#include <marlin/core/transports/TransportFactoryScaffold.hpp>
#include "StreamTransport.hpp"

namespace marlin {
namespace stream {

/// @brief Factory class to create and manage StreamTranport instances providing stream semantics.
///
/// Wraps around a base transport factory providing datagram semantics.
/// Exposes functions to bind to a socket, listening to incoming connections and dialing to a peer.
template<
	typename ListenDelegate,
	typename TransportDelegate,
	template<typename, typename> class DatagramTransportFactory,
	template<typename> class DatagramTransport
>
class StreamTransportFactory : public core::SugaredTransportFactoryScaffold<
	ListenDelegate,
	TransportDelegate,
	DatagramTransportFactory,
	DatagramTransport,
	StreamTransportFactory,
	StreamTransport
> {
public:
	using TransportFactoryScaffoldType = core::SugaredTransportFactoryScaffold<
		ListenDelegate,
		TransportDelegate,
		DatagramTransportFactory,
		DatagramTransport,
		StreamTransportFactory,
		StreamTransport
	>;
private:
	using TransportFactoryScaffoldType::base_factory;
	using TransportFactoryScaffoldType::transport_manager;

public:
	using TransportFactoryScaffoldType::addr;

	using TransportFactoryScaffoldType::TransportFactoryScaffoldType;

	using TransportFactoryScaffoldType::bind;
	using TransportFactoryScaffoldType::listen;
	using TransportFactoryScaffoldType::dial;

	using TransportFactoryScaffoldType::get_transport;
};

} // namespace stream
} // namespace marlin

#endif // MARLIN_STREAM_STREAMTRANSPORTFACTORY_HPP
