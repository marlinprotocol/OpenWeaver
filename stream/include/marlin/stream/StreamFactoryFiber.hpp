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
	using SelfType = StreamFactoryFiber<ExtFabric>;
	using FiberScaffoldType = FiberScaffold<
		SelfType,
		ExtFabric,
		Buffer,
		Buffer
	>;

	using typename FiberScaffoldType::InnerMessageType;
	using typename FiberScaffoldType::OuterMessageType;

	using FiberScaffoldType::FiberScaffoldType;


	int did_recv(
		auto &&,	// id of the Stream
		InnerMessageType&&,
		SocketAddress
	) {
		// Process
		SPDLOG_INFO("Received something in stream factory fiber.");
		return 0;
	}
};

}
}

#endif