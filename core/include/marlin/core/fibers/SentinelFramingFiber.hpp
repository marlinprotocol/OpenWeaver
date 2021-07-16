#ifndef MARLIN_CORE_FIBERS_SENTINELFRAMINGFIBER_HPP
#define MARLIN_CORE_FIBERS_SENTINELFRAMINGFIBER_HPP

#include <marlin/core/Buffer.hpp>
#include <marlin/core/fibers/FiberScaffold.hpp>

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

	int did_recv(auto&& source, InnerMessageType&& bytes, SocketAddress addr) {
		// get idx of sentinel if present
		size_t sentinel_idx = std::find(
			bytes.data(),
			bytes.data() + bytes.size(),
			sentinel
		) - bytes.data();

		if(sentinel_idx == bytes.size()) {
			// sentinel not found, just forward
			return FiberScaffoldType::did_recv(*this, std::move(bytes), addr);
		}

		// sentinel found
		// copy and send
		core::Buffer n_bytes(sentinel_idx+1);
		bytes.read_unsafe(0, n_bytes.data(), sentinel_idx+1);
		auto res = FiberScaffoldType::did_recv(*this, std::move(n_bytes), addr);
		if(res < 0) {
			return res;
		}

		// notify sentinel
		this->ext_fabric.i(*this).did_recv_sentinel(this->ext_fabric.is(*this), addr);

		// report leftover if any
		bytes.cover_unsafe(sentinel_idx+1);
		if(bytes.size() > 0) {
			return source.leftover(*this, std::move(bytes), addr);
		}

		return 0;
	}
};

}  // namespace core
}  // namespace marlin

#endif  // MARLIN_CORE_FIBERS_SENTINELFRAMINGFIBER_HPP
