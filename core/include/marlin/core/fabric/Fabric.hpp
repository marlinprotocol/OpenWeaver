#ifndef MARLIN_CORE_FABRIC_FABRIC_HPP
#define MARLIN_CORE_FABRIC_FABRIC_HPP

#include <marlin/core/Buffer.hpp>


namespace marlin {
namespace core {

// Fibers assumed to be ordered from Outer to Inner
template<typename... Fibers>
class Fabric {
	struct Empty {};

	// Important: Not zero indexed!
	template<size_t n>
		//requires (n <= sizeof...(Fibers))
	using NthFiber = typename std::tuple_element<n, std::tuple<Empty, Fibers...>>::type;

	template<typename FiberOuter, typename FiberInner>
	static constexpr bool fits_binary() {
		return (
			// Outer fiber must be open on the inner side
			FiberOuter::is_inner_open &&
			// Inner fiber must be open on the outer side
			FiberInner::is_outer_open &&
			// MessageType must be compatible
			std::is_same_v<typename FiberOuter::InnerMessageType, typename FiberInner::OuterMessageType>
		);
	}

	template<size_t... Is>
	static constexpr bool fits(std::index_sequence<Is...>) {
		// fold expression
		return (... && fits_binary<NthFiber<Is+1>, NthFiber<Is+2>>());
	}

	// Assert that all fibers fit well together
	static_assert(fits(std::make_index_sequence<sizeof...(Fibers)-1>{}));

private:
	// Important: Not zero indexed!
	std::tuple<Empty, Fibers...> fibers;
public:
	using OuterMessageType = typename NthFiber<1>::OuterMessageType;
	using InnerMessageType = typename NthFiber<sizeof...(Fibers)>::InnerMessageType;

	static constexpr bool is_outer_open = NthFiber<1>::is_outer_open;
	static constexpr bool is_inner_open = NthFiber<sizeof...(Fibers)>::is_inner_open;

	// Guide to fiber index
	// 0						call on external fabric
	// [1,len(Fibers)]			call on fiber
	// len(Fibers)+1			call on external fabric

	template<typename FabricType, typename... Args, size_t idx = 1>
		requires (
			// idx should be in range
			idx <= sizeof...(Fibers) + 1 &&
			// Should never have idx 0
			idx != 0 &&
			// Should never be called with idx 1 if outermost fiber is closed on the outer side
			!(idx == 1 && !NthFiber<1>::is_outer_open) &&
			// Should never call external fabric if innermost fiber is closed on the inner side
			!(idx == sizeof...(Fibers) + 1 && !NthFiber<sizeof...(Fibers)>::is_inner_open)
		)
	int did_recv(FabricType&&, Buffer&&, Args&&...) {
		return 0;
	}
};

}  // namespace core
}  // namespace marlin

#endif  // MARLIN_CORE_FABRIC_FABRIC_HPP
