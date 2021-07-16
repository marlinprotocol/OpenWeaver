#ifndef MARLIN_CORE_FIBERS_SENTINELBUFFERFIBER_HPP
#define MARLIN_CORE_FIBERS_SENTINELBUFFERFIBER_HPP

#include <marlin/core/Buffer.hpp>
#include <marlin/core/fibers/FiberScaffold.hpp>

namespace marlin {
namespace core {

template<typename ExtFabric>
class SentinelBufferFiber : public FiberScaffold<
	SentinelBufferFiber<ExtFabric>,
	ExtFabric,
	Buffer,
	Buffer
> {
public:
	using SelfType = SentinelBufferFiber<ExtFabric>;
	using FiberScaffoldType = FiberScaffold<
		SelfType,
		ExtFabric,
		Buffer,
		Buffer
	>;

	using typename FiberScaffoldType::InnerMessageType;
	using typename FiberScaffoldType::OuterMessageType;

	using FiberScaffoldType::FiberScaffoldType;

	core::Buffer buf = core::Buffer(0);

	void reset(uint64_t max_len) {
		buf = core::Buffer(max_len);
		buf.truncate_unsafe(max_len);
	}

	int did_recv(auto&&, InnerMessageType&& bytes, SocketAddress) {
		auto idx = buf.size();
		// expand and check if there is enough room to copy
		if(!buf.expand(bytes.size())) {
			// no room, error and return
			return -1;
		}

		// have room, copy
		buf.write_unsafe(idx, bytes.data(), bytes.size());

		return 0;
	}

	int did_recv_sentinel(auto&&, SocketAddress addr) {
		// pass on msg
		return FiberScaffoldType::did_recv(*this, std::move(buf), addr);
	}
};

}  // namespace core
}  // namespace marlin

#endif  // MARLIN_CORE_FIBERS_SENTINELBUFFERFIBER_HPP
