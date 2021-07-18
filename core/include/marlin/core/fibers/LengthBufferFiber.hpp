#ifndef MARLIN_CORE_FIBERS_LENGTHBUFFERFIBER_HPP
#define MARLIN_CORE_FIBERS_LENGTHBUFFERFIBER_HPP

#include <marlin/core/Buffer.hpp>
#include <marlin/core/fibers/FiberScaffold.hpp>

namespace marlin {
namespace core {

template<typename ExtFabric>
class LengthBufferFiber : public FiberScaffold<
	LengthBufferFiber<ExtFabric>,
	ExtFabric,
	Buffer,
	Buffer
> {
public:
	using SelfType = LengthBufferFiber<ExtFabric>;
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
		this->ext_fabric.o(*this).reset(max_len);
	}

	int did_recv(auto&&, InnerMessageType&& bytes, uint64_t bytes_remaining, SocketAddress) {
		auto idx = buf.size() - bytes.size() - bytes_remaining;

		// copy
		buf.write_unsafe(idx, bytes.data(), bytes.size());

		return 0;
	}

	int did_recv_frame(auto&&, SocketAddress addr) {
		// pass on msg
		return FiberScaffoldType::did_recv(*this, std::move(buf), addr);
	}
};

}  // namespace core
}  // namespace marlin

#endif  // MARLIN_CORE_FIBERS_LENGTHBUFFERFIBER_HPP
