#ifndef MARLIN_CORE_FIBERS_FRAMINGBASEFIBER_HPP
#define MARLIN_CORE_FIBERS_FRAMINGBASEFIBER_HPP

#include <marlin/core/Buffer.hpp>
#include <marlin/core/fibers/FiberScaffold.hpp>
#include <marlin/core/SocketAddress.hpp>


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

	template<uint32_t tag>
	auto inner_call(auto&&... args) {
		if constexpr (tag == "leftover"_tag) {
			return reset(std::forward<decltype(args)>(args)...);
		} else {
			return FiberScaffoldType::template inner_call<tag>(std::forward<decltype(args)>(args)...);
		}
	}

private:
	int leftover(auto&&, InnerMessageType&& bytes, SocketAddress addr) {
		return source.template outer_call<"did_recv"_tag>(*this, std::move(bytes), addr);
	}
};

}  // namespace core
}  // namespace marlin

#endif  // MARLIN_CORE_FIBERS_FRAMINGBASEFIBER_HPP
