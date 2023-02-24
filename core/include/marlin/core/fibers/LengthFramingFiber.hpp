#ifndef MARLIN_CORE_FIBERS_LENGTHFRAMINGFIBER_HPP
#define MARLIN_CORE_FIBERS_LENGTHFRAMINGFIBER_HPP

#include <marlin/core/Buffer.hpp>
#include <marlin/core/fibers/FiberScaffold.hpp>
#include <marlin/core/SocketAddress.hpp>


namespace marlin {
namespace core {

template<typename ExtFabric>
class LengthFramingFiber : public FiberScaffold<
	LengthFramingFiber<ExtFabric>,
	ExtFabric,
	Buffer,
	Buffer
> {
public:
	using SelfType = LengthFramingFiber<ExtFabric>;
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
	uint64_t bytes_remaining = 0;

	void reset(uint64_t length) {
		bytes_remaining = length;
	}

	int did_recv(auto&&, auto&& source, InnerMessageType&& bytes, SocketAddress addr) {
		// check if entire frame available
		if(bytes.size() < bytes_remaining) {
			// nope, forward entirely
			bytes_remaining -= bytes.size();
			return this->ext_fabric.template inner_call<"did_recv"_tag>(
				*this,
				*this,
				std::move(bytes),
				bytes_remaining,
				addr
			);
		}

		// copy leftovers and truncate
		auto num_leftover = bytes.size() - bytes_remaining;
		auto leftover = core::Buffer(num_leftover);
		leftover.write_unsafe(0, bytes.data() + bytes_remaining, num_leftover);
		bytes.truncate_unsafe(num_leftover);
		bytes_remaining = 0;

		// send
		auto res = this->ext_fabric.template inner_call<"did_recv"_tag>(
			*this,
			*this,
			std::move(bytes),
			bytes_remaining,
			addr
		);
		if(res < 0) {
			return res;
		}

		// notify full frame
		this->ext_fabric.template inner_call<"did_recv_frame"_tag>(*this, *this, addr);

		// report leftover if any
		if(num_leftover > 0) {
			return source.template inner_call<"leftover"_tag>(*this, std::move(leftover), addr);
		}

		return 0;
	}
};

}  // namespace core
}  // namespace marlin

#endif  // MARLIN_CORE_FIBERS_LENGTHFRAMINGFIBER_HPP
