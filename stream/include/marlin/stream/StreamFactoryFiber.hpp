#ifndef MARLIN_ASYNCIO_UDP_STREAMFACTORYFIBER_HPP
#define MARLIN_ASYNCIO_UDP_STREAMFACTORYFIBER_HPP

#include <marlin/core/Buffer.hpp>
#include <marlin/core/SocketAddress.hpp>
#include <spdlog/spdlog.h>
#include <marlin/core/fibers/FiberScaffold.hpp>



using namespace marlin::core;

namespace marlin {
namespace stream {

template <typename ExtFabric>
class StreamFactoryFiber : public FiberScaffold<
	StreamFactoryFiber<ExtFabric>,
	ExtFabric,
	Buffer,
	Buffer
>  {
public:
	static constexpr bool is_inner_open = true;
	static constexpr bool is_outer_open = true;

	using SelfType = StreamFactoryFiber<ExtFabric>;

	using InnerMessageType = Buffer;
	using OuterMessageType = Buffer;

	[[no_unique_address]] ExtFabric ext_fabric;

	// template<typename... Args>
	// StreamFactoryFiber(Args&&...) {}

	int did_recv(
		auto &&,	// id of the Stream
		Buffer&&,
		SocketAddress
	) {
		// Process
	}
};

}
}

#endif