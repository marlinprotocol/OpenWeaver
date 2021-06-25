#ifndef MARLIN_CORE_FABRIC_FABRIC_HPP
#define MARLIN_CORE_FABRIC_FABRIC_HPP

#include <marlin/core/Buffer.hpp>
#include <marlin/core/SocketAddress.hpp>


namespace marlin {
namespace core {

namespace {

template<size_t idx, template<typename> typename FiberTemplate, template<typename> typename... FiberTemplates>
	requires (idx != 0)
struct NthFiberHelper {
	template<typename T>
	using type = typename NthFiberHelper<idx-1, FiberTemplates...>::template type<T>;
};

template<template<typename> typename FiberTemplate, template<typename> typename... FiberTemplates>
struct NthFiberHelper<1, FiberTemplate, FiberTemplates...> {
	template<typename T>
	using type = FiberTemplate<T>;
};


template<typename T, typename Tuple>
struct TupleCat {};

template<typename T, typename... TupleTypes>
struct TupleCat<T, std::tuple<TupleTypes...>> {
	using type = std::tuple<T, TupleTypes...>;
};

template<size_t idx, size_t total, template<size_t> typename Shuttle, template<typename> typename FiberTemplate, template<typename> typename... FiberTemplates>
struct TupleHelper {
	using base = typename TupleHelper<idx+1, total, Shuttle, FiberTemplates...>::type;
	using type = typename TupleCat<FiberTemplate<Shuttle<idx>>, base>::type;
};

template<size_t total, template<size_t> typename Shuttle, template<typename> typename FiberTemplate, template<typename> typename... FiberTemplates>
struct TupleHelper<0, total, Shuttle, FiberTemplate, FiberTemplates...> {
	struct Empty {
		template<typename... Args>
		Empty(Args&&...) {}
	};
	using base = typename TupleHelper<1, total, Shuttle, FiberTemplate, FiberTemplates...>::type;
	using type = typename TupleCat<std::tuple<Empty>, base>::type;
};

template<size_t idx, template<size_t> typename Shuttle, template<typename> typename FiberTemplate, template<typename> typename... FiberTemplates>
struct TupleHelper<idx, idx, Shuttle, FiberTemplate, FiberTemplates...> {
	using type = std::tuple<FiberTemplate<Shuttle<idx>>>;
};

}

// Fibers assumed to be ordered from Outer to Inner
template<typename ExtFabric, template<typename> typename... FiberTemplates>
class Fabric {
public:
	using SelfType = Fabric<ExtFabric, FiberTemplates...>;

private:
	// Forward declaration
	template<size_t idx>
	struct Shuttle;

	// Important: Not zero indexed!
	template<size_t idx>
		//requires (idx <= sizeof...(FiberTemplates))
	using NthFiber = typename NthFiberHelper<idx, FiberTemplates...>::template type<Shuttle<idx>>;

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
	static_assert(fits(std::make_index_sequence<sizeof...(FiberTemplates)-1>{}));

	// External fabric
	[[no_unique_address]] ExtFabric ext_fabric;

	// Important: Not zero indexed!
	[[no_unique_address]] typename TupleHelper<
		0,
		sizeof...(FiberTemplates),
		Shuttle,
		FiberTemplates...
	>::type fibers;


	// Private constructor
	template<typename ExtTupleType, typename... TupleTypes, size_t... Is>
	Fabric(std::tuple<ExtTupleType, TupleTypes...>&& init_tuple, std::index_sequence<Is...>) :
		ext_fabric(std::forward<ExtTupleType>(std::get<0>(init_tuple))),
		fibers(
			// Empty
			std::make_tuple(),
			// Other fibers
			std::move(std::tuple_cat(
				// Shuttle
				std::make_tuple(std::make_tuple()),
				// Init
				std::move(std::get<Is+1>(init_tuple))
			))...
		)
	{}

public:
	// Public constructor
	template<typename ExtTupleType, typename... TupleTypes>
	Fabric(std::tuple<ExtTupleType, TupleTypes...>&& init_tuple) :
		Fabric(std::move(init_tuple), std::index_sequence_for<TupleTypes...>())
	{}

private:
	// Warning: Potentially very brittle
	// Calculate offset of fabric from reference to fiber
	template<size_t idx>
	static constexpr SelfType& get_fabric(NthFiber<idx>& fiber_ptr) {
		// Type cast from nullptr
		// Other option is to declare local var, but forces default constructible
		auto* ref_fabric_ptr = (SelfType*)nullptr;
		auto* ref_fiber_ptr = &std::get<idx>(ref_fabric_ptr->fibers);

		return *(SelfType*)((uint8_t*)&fiber_ptr - ((uint8_t*)ref_fiber_ptr - (uint8_t*)ref_fabric_ptr));
	}

	// Shuttle for properly transitioning between fibers
	template<size_t idx>
	struct Shuttle {
		static_assert(idx != 0 && idx <= sizeof...(FiberTemplates));

		template<typename... Args>
		Shuttle(Args&&...) {}

		auto& i(NthFiber<idx>& caller) {
			// Warning: Requires that caller is fiber at idx
			auto& fabric = get_fabric<idx>(caller);

			// Check for exit first
			if constexpr (idx == sizeof...(FiberTemplates)) {
				// inside shuttle of last fiber, exit
				return fabric.ext_fabric.i(fabric);
			} else {
				// Transition to next fiber
				return std::get<idx+1>(fabric.fibers);
			}
		}

		auto& o(NthFiber<idx>& caller) {
			// Warning: Requires that caller is fiber at idx
			auto& fabric = get_fabric<idx>(caller);

			// Check for exit first
			if constexpr (idx == 1) {
				// inside shuttle of last fiber, exit
				return fabric.ext_fabric.o(fabric);
			} else {
				// Transition to next fiber
				return std::get<idx-1>(fabric.fibers);
			}
		}

		template<typename... Args>
		int did_recv(NthFiber<idx>& caller, Args&&... args) {
			// Warning: Requires that caller is fiber at idx
			auto& fabric = get_fabric<idx>(caller);

			// Check for exit first
			if constexpr (idx == sizeof...(FiberTemplates)) {
				// inside shuttle of last fiber, exit
				return fabric.ext_fabric.did_recv(fabric, std::forward<Args>(args)...);
			} else {
				// Transition to next fiber
				auto& next_fiber = std::get<idx+1>(fabric.fibers);
				return next_fiber.did_recv(std::forward<Args>(args)...);
			}
		}

		template<typename... Args>
		int did_dial(NthFiber<idx>& caller, Args&&... args) {
			// Warning: Requires that caller is fiber at idx
			auto& fabric = get_fabric<idx>(caller);

			// Check for exit first
			if constexpr (idx == sizeof...(FiberTemplates)) {
				// inside shuttle of last fiber, exit
				return fabric.ext_fabric.did_dial(fabric, std::forward<Args>(args)...);
			} else {
				// Transition to next fiber
				auto& next_fiber = std::get<idx+1>(fabric.fibers);
				return next_fiber.did_dial(std::forward<Args>(args)...);
			}
		}

		template<typename... Args>
		int did_send(NthFiber<idx>& caller, Args&&... args) {
			// Warning: Requires that caller is fiber at idx
			auto& fabric = get_fabric<idx>(caller);

			// Check for exit first
			if constexpr (idx == sizeof...(FiberTemplates)) {
				// inside shuttle of last fiber, exit
				return fabric.ext_fabric.did_send(fabric, std::forward<Args>(args)...);
			} else {
				// Transition to next fiber
				auto& next_fiber = std::get<idx+1>(fabric.fibers);
				return next_fiber.did_send(std::forward<Args>(args)...);
			}
		}

		template<typename... Args>
		int send(NthFiber<idx>& caller, Args&&... args) {
			// Warning: Requires that caller is fiber at idx
			auto& fabric = get_fabric<idx>(caller);

			// Check for exit first
			if constexpr (idx == 1) {
				// inside shuttle of first fiber, exit
				return fabric.ext_fabric.send(fabric, std::forward<Args>(args)...);
			} else {
				// Transition to next fiber
				auto& next_fiber = std::get<idx-1>(fabric.fibers);
				return next_fiber.send(std::forward<Args>(args)...);
			}
		}
	};

public:
	using OuterMessageType = typename NthFiber<1>::OuterMessageType;
	using InnerMessageType = typename NthFiber<sizeof...(FiberTemplates)>::InnerMessageType;

	static constexpr bool is_outer_open = NthFiber<1>::is_outer_open;
	static constexpr bool is_inner_open = NthFiber<sizeof...(FiberTemplates)>::is_inner_open;

	// Guide to fiber index
	// 0						call on external fabric
	// [1,len(Fibers)]			call on fiber
	// len(Fibers)+1			call on external fabric

	template<
		typename OMT = OuterMessageType,
		typename... Args
	>
		requires (
			// Should only be called if fabric is open on the outer side
			is_outer_open
		)
	int did_recv(OMT&& buf, Args&&... args) {
		return std::get<1>(fibers).did_recv(std::move(buf), std::forward<Args>(args)...);
	}

	template<bool is_open = is_outer_open>
		requires (!is_open)
	int bind(SocketAddress const& addr) {
		return std::get<1>(fibers).bind(addr);
	}

	template<bool is_open = is_outer_open>
		requires (!is_open)
	int listen() {
		return std::get<1>(fibers).listen();
	}

	template<bool is_open = is_outer_open>
		requires (!is_open)
	int dial(core::SocketAddress addr, auto&&... args) {
		return std::get<1>(fibers).dial(addr, std::forward<decltype(args)>(args)...);
	}

	template<
		typename IMT = InnerMessageType,
		typename... Args
	>
		requires (
			// Should only be called if fabric is open on the inner side
			is_inner_open
		)
	int send(InnerMessageType&& buf, core::SocketAddress addr) {
		return std::get<sizeof...(FiberTemplates)>(fibers).send(std::move(buf), addr);
	}

	// Stub
	SelfType& i(auto&&...) {
		return *this;
	}

	// Stub
	SelfType& o(auto&&...) {
		return *this;
	}
};


template<template<typename> typename... F>
struct FabricF {
	template<typename T>
	using type = Fabric<T, F...>;
};

}  // namespace core
}  // namespace marlin

#endif  // MARLIN_CORE_FABRIC_FABRIC_HPP
