#ifndef MARLIN_CORE_VERSIONEDTRANSPORTFACTORY_HPP
#define MARLIN_CORE_VERSIONEDTRANSPORTFACTORY_HPP

#include <marlin/core/transports/TransportFactoryScaffold.hpp>
#include <marlin/core/transports/VersionedTransport.hpp>
#include <marlin/core/Buffer.hpp>
#include <marlin/core/SocketAddress.hpp>

#include <spdlog/spdlog.h>

namespace marlin {
namespace core {

template<
	typename ListenDelegate,
	typename TransportDelegate,
	template<typename, typename> class BaseTransportFactoryTemplate,
	template<typename> class BaseTransportTemplate
>
class VersionedTransportFactory : public TransportFactoryScaffold<
	VersionedTransportFactory<ListenDelegate, TransportDelegate, BaseTransportFactoryTemplate, BaseTransportTemplate>,
	VersionedTransport<TransportDelegate, BaseTransportTemplate>,
	ListenDelegate,
	TransportDelegate,
	BaseTransportFactoryTemplate<VersionedTransportFactory<ListenDelegate, TransportDelegate, BaseTransportFactoryTemplate, BaseTransportTemplate>, VersionedTransport<TransportDelegate, BaseTransportTemplate>>,
	BaseTransportTemplate<VersionedTransport<TransportDelegate, BaseTransportTemplate>>&
> {
public:
	using SelfType = VersionedTransportFactory<ListenDelegate, TransportDelegate, BaseTransportFactoryTemplate, BaseTransportTemplate>;
	using SelfTransportType = VersionedTransport<TransportDelegate, BaseTransportTemplate>;
	using BaseTransportFactoryType = BaseTransportFactoryTemplate<SelfType, SelfTransportType>;
	using BaseTransportType = BaseTransportTemplate<SelfTransportType>;
	using TransportFactoryScaffoldType = TransportFactoryScaffold<
		SelfType, SelfTransportType, ListenDelegate, TransportDelegate, BaseTransportFactoryType, BaseTransportType&
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

} // namespace core
} // namespace marlin

#endif // MARLIN_CORE_VERSIONEDTRANSPORTFACTORY_HPP
