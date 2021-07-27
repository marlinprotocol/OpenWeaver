#ifndef MARLIN_CORE_FIBERS_FRAMINGBASEFIBER_HPP
#define MARLIN_CORE_FIBERS_FRAMINGBASEFIBER_HPP

#include <marlin/core/Buffer.hpp>
#include <marlin/core/fibers/FiberScaffold.hpp>

namespace marlin {
namespace core {

template<typename ExtFabric>
class FramingBaseFiber : public FiberScaffold<
	FramingBaseFiber<ExtFabric>,
	ExtFabric,
	Buffer,
	Buffer
> {
public:
	using SelfType = FramingBaseFiber<ExtFabric>;
	using FiberScaffoldType = FiberScaffold<
		SelfType,
		ExtFabric,
		Buffer,
		Buffer
	>;

	using typename FiberScaffoldType::InnerMessageType;
	using typename FiberScaffoldType::OuterMessageType;

	using FiberScaffoldType::FiberScaffoldType;

	int leftover(auto&& source, InnerMessageType&& bytes, SocketAddress addr) {
		return source.did_recv(this->ext_fabric.is(*this), std::move(bytes), addr);
	}
};

}  // namespace core
}  // namespace marlin

#endif  // MARLIN_CORE_FIBERS_FRAMINGBASEFIBER_HPP
