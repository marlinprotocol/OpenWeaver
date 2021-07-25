#ifndef MARLIN_CORE_FIBERS_LENGTHFRAMINGFIBER_HPP
#define MARLIN_CORE_FIBERS_LENGTHFRAMINGFIBER_HPP

#include <marlin/core/Buffer.hpp>
#include <marlin/core/fibers/FiberScaffold.hpp>

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

	uint64_t bytes_remaining = 0;

	void reset(uint64_t length) {
		bytes_remaining = length;
	}

	int did_recv(auto&& source, InnerMessageType&& bytes, SocketAddress addr) {
		// check if entire frame available
		if(bytes.size() < bytes_remaining) {
			// nope, forward entirely
			bytes_remaining -= bytes.size();
			return this->ext_fabric.i(*this).did_recv(
				this->ext_fabric.is(*this),
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
		auto res = this->ext_fabric.i(*this).did_recv(
			this->ext_fabric.is(*this),
			std::move(bytes),
			bytes_remaining,
			addr
		);
		if(res < 0) {
			return res;
		}

		// notify full frame
		this->ext_fabric.i(*this).did_recv_frame(this->ext_fabric.is(*this), addr);

		// report leftover if any
		if(num_leftover > 0) {
			return source.leftover(*this, std::move(leftover), addr);
		}

		return 0;
	}
};

}  // namespace core
}  // namespace marlin

#endif  // MARLIN_CORE_FIBERS_LENGTHFRAMINGFIBER_HPP
