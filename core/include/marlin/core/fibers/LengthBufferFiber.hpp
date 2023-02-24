#ifndef MARLIN_CORE_FIBERS_LENGTHBUFFERFIBER_HPP
#define MARLIN_CORE_FIBERS_LENGTHBUFFERFIBER_HPP

#include <marlin/core/Buffer.hpp>
#include <marlin/core/fibers/FiberScaffold.hpp>
#include <marlin/core/SocketAddress.hpp>


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

	template<uint32_t tag>
	auto inner_call(auto&&... args) {
		if constexpr (tag == "did_recv"_tag) {
			return did_recv(std::forward<decltype(args)>(args)...);
		} else if constexpr (tag == "did_recv_frame"_tag) {
			return did_recv_frame(std::forward<decltype(args)>(args)...);
		} else {
			// static_assert(false) always breaks compilation
			// making it depend on a template parameter fixes it
			static_assert(tag < 0);
		}
	}

	template<uint32_t tag>
	auto outer_call(auto&&... args) {
		if constexpr (tag == "reset"_tag) {
			return reset(std::forward<decltype(args)>(args)...);
		} else {
			// static_assert(false) always breaks compilation
			// making it depend on a template parameter fixes it
			static_assert(tag < 0);
		}
	}

private:
	core::Buffer buf = core::Buffer(0);

	void reset(uint64_t max_len) {
		buf = core::Buffer(max_len);
		this->ext_fabric.template outer_call<"reset"_tag>(max_len);
	}

	int did_recv(auto&&, auto&&, InnerMessageType&& bytes, uint64_t bytes_remaining, SocketAddress) {
		auto idx = buf.size() - bytes.size() - bytes_remaining;

		// copy
		buf.write_unsafe(idx, bytes.data(), bytes.size());

		return 0;
	}

	int did_recv_frame(auto&&, auto&&, SocketAddress addr) {
		// pass on msg
		return FiberScaffoldType::template inner_call<"did_recv"_tag>(*this, *this, std::move(buf), addr);
	}
};

}  // namespace core
}  // namespace marlin

#endif  // MARLIN_CORE_FIBERS_LENGTHBUFFERFIBER_HPP
