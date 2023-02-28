#ifndef MARLIN_CORE_FIBERS_SENTINELFRAMINGFIBER_HPP
#define MARLIN_CORE_FIBERS_SENTINELFRAMINGFIBER_HPP

#include <marlin/core/Buffer.hpp>
#include <marlin/core/fibers/FiberScaffold.hpp>
#include <marlin/core/SocketAddress.hpp>

#include <spdlog/spdlog.h>


namespace marlin {
namespace core {

template<typename ExtFabric, uint8_t sentinel>
class SentinelFramingFiber : public FiberScaffold<
	SentinelFramingFiber<ExtFabric, sentinel>,
	ExtFabric,
	Buffer,
	Buffer
> {
public:
	using SelfType = SentinelFramingFiber<ExtFabric, sentinel>;
	using FiberScaffoldType = FiberScaffold<
		SelfType,
		ExtFabric,
		Buffer,
		Buffer
	>;

	using typename FiberScaffoldType::InnerMessageType;
	using typename FiberScaffoldType::OuterMessageType;

	using FiberScaffoldType::FiberScaffoldType;

	template<uint32_t tag>
	auto outer_call(auto&&... args) {
		if constexpr (tag == "did_recv"_tag) {
			return did_recv(std::forward<decltype(args)>(args)...);
		} else {
			// static_assert(false) always breaks compilation
			// making it depend on a template parameter fixes it
			static_assert(tag < 0);
		}
	}

private:
	int did_recv(auto&& source, InnerMessageType&& bytes, SocketAddress addr) {
		SPDLOG_DEBUG("SFF: did_recv: {} bytes", bytes.size());
		// get idx of sentinel if present
		size_t sentinel_idx = std::find(
			bytes.data(),
			bytes.data() + bytes.size(),
			sentinel
		) - bytes.data();

		if(sentinel_idx == bytes.size()) {
			// sentinel not found, just forward
			return FiberScaffoldType::template outer_call<"did_recv"_tag>(*this, std::move(bytes), addr);
		}

		// sentinel found
		// copy and send
		core::Buffer n_bytes(sentinel_idx+1);
		bytes.read_unsafe(0, n_bytes.data(), sentinel_idx+1);
		auto res = FiberScaffoldType::template outer_call<"did_recv"_tag>(*this, std::move(n_bytes), addr);
		if(res < 0) {
			return res;
		}

		// notify sentinel
		FiberScaffoldType::template outer_call<"did_recv_sentinel"_tag>(*this, addr);

		// report leftover if any
		bytes.cover_unsafe(sentinel_idx+1);
		if(bytes.size() > 0) {
			return source.template inner_call<"leftover"_tag>(*this, std::move(bytes), addr);
		}

		return 0;
	}
};

}  // namespace core
}  // namespace marlin

#endif  // MARLIN_CORE_FIBERS_SENTINELFRAMINGFIBER_HPP
