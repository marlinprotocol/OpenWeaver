#ifndef MARLIN_LPF_LPFTRANSPORTFACTORY_HPP
#define MARLIN_LPF_LPFTRANSPORTFACTORY_HPP

#include <marlin/core/transports/TransportFactoryScaffold.hpp>
#include "LpfTransport.hpp"


namespace marlin {
namespace lpf {

template<
	typename ListenDelegate,
	typename TransportDelegate,
	template<typename, typename> class StreamTransportFactory,
	template<typename> class StreamTransport,
	typename SHOULD_CUT_THROUGH = std::false_type,
	typename PREFIX_LENGTH = std::integral_constant<uint8_t, 8>
>
class LpfTransportFactory : public core::SugaredTransportFactoryScaffold<
	ListenDelegate,
	TransportDelegate,
	StreamTransportFactory,
	StreamTransport,
	LpfTransportFactory,
	LpfTransport,
	SHOULD_CUT_THROUGH,
	PREFIX_LENGTH
> {
public:
	using TransportFactoryScaffoldType = core::SugaredTransportFactoryScaffold<
		ListenDelegate,
		TransportDelegate,
		StreamTransportFactory,
		StreamTransport,
		LpfTransportFactory,
		LpfTransport,
		SHOULD_CUT_THROUGH,
		PREFIX_LENGTH
	>;

	static constexpr bool should_cut_through = SHOULD_CUT_THROUGH::value;
	static constexpr uint8_t prefix_length = PREFIX_LENGTH::value;
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

} // namespace lpf
} // namespace marlin

#endif // MARLIN_LPF_LPFTRANSPORTFACTORY_HPP
