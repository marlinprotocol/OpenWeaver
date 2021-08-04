#ifndef MARLIN_MATIC_ABCI
#define MARLIN_MATIC_ABCI

#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <marlin/core/Buffer.hpp>
#include <marlin/core/fibers/DynamicFramingFiber.hpp>
#include <marlin/core/fibers/SentinelFramingFiber.hpp>
#include <marlin/core/fibers/SentinelBufferFiber.hpp>
#include <marlin/core/fibers/LengthFramingFiber.hpp>
#include <marlin/core/fibers/LengthBufferFiber.hpp>
#include <marlin/asyncio/core/Timer.hpp>
#include <marlin/asyncio/tcp/RcTcpOutFiber.hpp>


namespace marlin {
namespace matic {

template<typename X>
using SentinelFramingFiberHelper = core::SentinelFramingFiber<X, '\n'>;

template<typename DelegateType, typename... MetadataTypes>
class Abci {
public:
	using SelfType = Abci<DelegateType, MetadataTypes...>;
private:
	using FiberType = core::Fabric<
		Abci&,
		asyncio::RcTcpOutFiber,
		core::DynamicFramingFiberHelper<
			core::FabricF<SentinelFramingFiberHelper, core::SentinelBufferFiber>::type,
			core::FabricF<core::LengthFramingFiber, core::LengthBufferFiber>::type
		>::type
	>;

	FiberType fiber;

	uint64_t connect_timer_interval = 1000;
	asyncio::Timer connect_timer;

	void connect_timer_cb() {
		(void)fiber.i(*this).dial(dst);
	}

	uint8_t state = 0;
	uint64_t length = 0;

	uint64_t id = 0;
	std::unordered_map<uint64_t, std::tuple<core::Buffer, MetadataTypes...>> block_store;
public:
	DelegateType* delegate;
	core::SocketAddress dst;

	Abci(std::string dst) : fiber(std::forward_as_tuple(
			*this,
			std::make_tuple(),
			std::make_tuple(std::make_tuple(
				std::make_tuple(),
				std::make_tuple()
			))
		)), connect_timer(this), dst(core::SocketAddress::from_string(dst)) {
		connect_timer_cb();
	}

	// Delegate
	int did_connect(FiberType& fiber);
	int did_recv(FiberType& fiber, core::Buffer&& bytes, core::SocketAddress addr);
	int did_disconnect(FiberType& fiber, uint reason);
	int did_close(FiberType& fiber);

	int close() {
		SPDLOG_INFO("Close");
		return fiber.close();
	}

	void get_block_number();
	template<typename... MT>
	uint64_t analyze_block(core::Buffer&& block, MT&&... metadata);
};

}  // namespace matic
}  // namespace marlin

// Impl
#include "Abci.ipp"

#endif  // MARLIN_MATIC_ABCI
