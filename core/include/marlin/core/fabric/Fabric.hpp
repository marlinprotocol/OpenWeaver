#ifndef MARLIN_CORE_FABRIC_FABRIC_HPP
#define MARLIN_CORE_FABRIC_FABRIC_HPP

#include <marlin/core/Buffer.hpp>
#include <marlin/core/SocketAddress.hpp>


namespace marlin {
namespace core {

namespace {

template<size_t idx, template<typename> typename FiberTemplate, template<typename> typename... FiberTemplates>
struct NthFiberHelper {
	template<typename T>
	using type = typename NthFiberHelper<idx-1, FiberTemplates...>::template type<T>;
};

template<template<typename> typename FiberTemplate, template<typename> typename... FiberTemplates>
struct NthFiberHelper<0, FiberTemplate, FiberTemplates...> {
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

	template<size_t idx>
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
		return (... && fits_binary<NthFiber<Is>, NthFiber<Is+1>>());
	}

	// Assert that all fibers fit well together
	static_assert(fits(std::make_index_sequence<sizeof...(FiberTemplates)-1>{}));

	// External fabric
	[[no_unique_address]] ExtFabric ext_fabric;

	[[no_unique_address]] typename TupleHelper<
		0,
		sizeof...(FiberTemplates)-1,
		Shuttle,
		FiberTemplates...
	>::type fibers;


	// Private constructor
	template<typename ExtTupleType, typename... TupleTypes, size_t... Is>
	Fabric(std::tuple<ExtTupleType, TupleTypes...>&& init_tuple, std::index_sequence<Is...>) :
		ext_fabric(std::forward<ExtTupleType>(std::get<0>(init_tuple))),
		fibers(
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
		static_assert(idx < sizeof...(FiberTemplates));

		template<typename... Args>
		Shuttle(Args&&...) {}

		auto& i(NthFiber<idx>& caller) {
			// Warning: Requires that caller is fiber at idx
			auto& fabric = get_fabric<idx>(caller);

			// Check for exit first
			if constexpr (idx == sizeof...(FiberTemplates) - 1) {
				// inside shuttle of last fiber, exit
				// recursive check
				if constexpr (requires (decltype(fabric.ext_fabric) f) {
					f.i(fabric);
				}) {
					return fabric.ext_fabric.i(fabric);
				} else {
					return fabric.ext_fabric;
				}
			} else {
				// transition to next fiber
				auto& fiber = std::get<idx+1>(fabric.fibers);

				// recursive check
				if constexpr (requires (decltype(fiber) f) {
					f.i(caller);
				}) {
					return fiber.i(caller);
				} else {
					return fiber;
				}
			}
		}

		template <typename... Args>
		auto& i(NthFiber<idx>& caller, Args... args) {
			auto& fabric = get_fabric<idx>(caller);

			if constexpr (idx == sizeof ...(FiberTemplates) - 1) {

				if constexpr (requires (decltype(fabric.ext_fabric) f) {
					f.i(fabric, std::forward <Args> (args)...);
				}) {
					return fabric.ext_fabric.i(fabric, std::forward <Args> (args)...);
				} else {
					return fabric.ext_fabric;
				}
			} else {

				auto& fiber = std::get <idx + 1> (fabric.fibers);
				SPDLOG_INFO("SOmething");
				if constexpr (requires (decltype(fiber) f) {
					f.i(fabric, std::forward <Args> (args)...);
				}) {
					SPDLOG_INFO("going to .i of switch");
					return fiber.i(fabric, std::forward <Args> (args)...);
				} else {
					return fiber;
				}
			}
		}

		auto& is(NthFiber<idx>& caller) {
			// Warning: Requires that caller is fiber at idx
			auto& fabric = get_fabric<idx>(caller);

			// Check for exit first
			if constexpr (idx == sizeof...(FiberTemplates) - 1) {
				// inside shuttle of last fiber, exit
				// recursive check
				if constexpr (requires (decltype(fabric.ext_fabric) f) {
					f.is(fabric);
				}) {
					return fabric.ext_fabric.is(fabric);
				} else {
					return fabric;
				}
			} else {
				return caller;
			}
		}

		auto& o(NthFiber<idx>& caller) {
			// Warning: Requires that caller is fiber at idx
			auto& fabric = get_fabric<idx>(caller);

			// Check for exit first
			if constexpr (idx == 0) {
				// inside shuttle of last fiber, exit
				// recursive check
				if constexpr (requires (decltype(fabric.ext_fabric) f) {
					f.o(fabric);
				}) {
					return fabric.ext_fabric.o(fabric);
				} else {
					return fabric.ext_fabric;
				}
			} else {
				// transition to next fiber
				auto& fiber = std::get<idx-1>(fabric.fibers);

				// recursive check
				if constexpr (requires (decltype(fiber) f) {
					f.o(caller);
				}) {
					return fiber.o(caller);
				} else {
					return fiber;
				}
			}
		}

		auto& os(NthFiber<idx>& caller) {
			// Warning: Requires that caller is fiber at idx
			auto& fabric = get_fabric<idx>(caller);

			// Check for exit first
			if constexpr (idx == 0) {
				// inside shuttle of last fiber, exit
				// recursive check
				if constexpr (requires (decltype(fabric.ext_fabric) f) {
					f.os(fabric);
				}) {
					return fabric.ext_fabric.os(fabric);
				} else {
					return fabric;
				}
			} else {
				return caller;
			}
		}
	};

public:
	using OuterMessageType = typename NthFiber<0>::OuterMessageType;
	using InnerMessageType = typename NthFiber<sizeof...(FiberTemplates)-1>::InnerMessageType;

	static constexpr bool is_outer_open = NthFiber<0>::is_outer_open;
	static constexpr bool is_inner_open = NthFiber<sizeof...(FiberTemplates)-1>::is_inner_open;

	auto& i(auto&&) {
		auto& fiber = std::get<0>(fibers);

		// recursive check
		if constexpr (requires (decltype(fiber) f) {
			f.i(*this);
		}) {
			return fiber.i(*this);
		} else {
			return fiber;
		}
	}

	auto& o(auto&&) {
		auto& fiber = std::get<sizeof...(FiberTemplates)-1>(fibers);

		// recursive check
		if constexpr (requires (decltype(fiber) f) {
			f.o(*this);
		}) {
			return fiber.o(*this);
		} else {
			return fiber;
		}
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
