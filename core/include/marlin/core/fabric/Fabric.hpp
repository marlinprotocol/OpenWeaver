#ifndef MARLIN_CORE_FABRIC_FABRIC_HPP
#define MARLIN_CORE_FABRIC_FABRIC_HPP

namespace marlin {
namespace core {

// Fibers assumed to be ordered from Outer to Inner
template<typename... Fibers>
class Fabric {

	// Important: Not zero indexed!
	template<int n>
		requires (n <= sizeof...(Fibers))
	using NthFiber = typename std::tuple_element<n, std::tuple<void, Fibers...>>::type;

	template<typename FiberOuter, typename FiberInner>
	static constexpr bool fits() {
		return (
			// Outer fiber must be open on the inner side
			FiberOuter::is_inner_open &&
			// Inner fiber must be open on the outer side
			FiberInner::is_outer_open &&
			// MessageType must be compatible
			std::is_same_v<FiberOuter::InnerMessageType, FiberInner::OuterMessageType>
		);
	}

	template<size_t... Is>
	static constexpr bool fits(std::index_sequence<Is...>) {
		// fold expression
		return ... && (Is == 0 || fits<NthFiber<Is>, NthFiber<Is+1>>());
	}

	// Assert that all fibers fit well together
	static_assert(fits(std::index_sequence_for<Is...>);

private:
	// Important: Not zero indexed!
	std::tuple<void, Fibers...> fibers;
public:
	using OuterMessageType = decltype(std::get<0>(stages))::OuterMessageType;
	using InnerMessageType = decltype(std::get<sizeof...(Fibers) - 1>(fibers))::InnerMessageType;

	template<typename FabricType, typename... Args, size_t idx = 1>
	int did_recv(FabricType&&, core::Buffer&&, Args&&...) {
		return 0;
	}
};

}  // namespace core
}  // namespace marlin

#endif  // MARLIN_CORE_FABRIC_FABRIC_HPP
