#include <marlin/core/fabric/Fabric.hpp>
#include <marlin/core/fibers/FiberScaffold.hpp>
#include <spdlog/spdlog.h>

using namespace marlin::core;


struct Terminal {
	static constexpr bool is_outer_open = false;
	static constexpr bool is_inner_open = false;

	using InnerMessageType = Buffer;
	using OuterMessageType = Buffer;

	Terminal(auto&&...) {}

	template<uint32_t tag>
	auto outer_call(auto&&... args) {
		if constexpr (tag == "did_recv"_tag) {
			return did_recv(std::forward<decltype(args)>(args)...);
		} else {
			// static_assert(false) always breaks compilation
			// making it depend on a template parameter fixes it
			static_assert(tag < 0);
		}
	}

private:
	int did_recv(auto&&, Buffer&&) {
		SPDLOG_INFO("Did recv: Terminal");
		return 0;
	}
};

template<typename ExtFabric>
struct Fiber {
	static constexpr bool is_outer_open = true;
	static constexpr bool is_inner_open = true;

	using InnerMessageType = Buffer;
	using OuterMessageType = Buffer;

	[[no_unique_address]] ExtFabric ext_fabric;
	int idx = 0;

	template<typename ExtTupleType>
	Fiber(std::tuple<ExtTupleType, int>&& init_tuple) :
		ext_fabric(std::get<0>(init_tuple)),
		idx(std::get<1>(init_tuple)) {}

	template<uint32_t tag>
	auto outer_call(auto&&... args) {
		if constexpr (tag == "did_recv"_tag) {
			return did_recv(std::forward<decltype(args)>(args)...);
		} else {
			// static_assert(false) always breaks compilation
			// making it depend on a template parameter fixes it
			static_assert(tag < 0);
		}
	}

private:
	int did_recv(auto&&, Buffer&& buf) {
		SPDLOG_INFO("Did recv: {}", idx);
		return ext_fabric.template outer_call<"did_recv"_tag>(*this, std::move(buf));
	}
};


int main() {
	// Simplest fabric
	Fabric<
		Terminal,
		Fiber
	> f_simplest(std::make_tuple(
		// Terminal
		std::make_tuple(std::make_tuple(), 0),
		// Other fibers
		std::make_tuple(1)
	));
	f_simplest.outer_call<"did_recv"_tag>(0, Buffer(5));

	// Multiple fibers fabric
	Fabric<
		Terminal,
		Fiber,
		Fiber,
		Fiber,
		Fiber,
		Fiber
	> f_multiple(std::make_tuple(
		// Terminal
		std::make_tuple(std::make_tuple(), 0),
		// Other fibers
		std::make_tuple(1),
		std::make_tuple(2),
		std::make_tuple(3),
		std::make_tuple(4),
		std::make_tuple(5)
	));
	f_multiple.outer_call<"did_recv"_tag>(0, Buffer(5));

	// Nested fabric
	Fabric<
		Terminal,
		FabricF<Fiber, Fiber>::type
	> f_nested(std::make_tuple(
		// Terminal
		std::make_tuple(std::make_tuple(), 0),
		// Other fibers
		std::make_tuple(std::make_tuple(1), std::make_tuple(2))
	));
	f_nested.outer_call<"did_recv"_tag>(0, Buffer(5));

	// Nested fabric
	Fabric<
		Terminal,
		Fiber,
		Fiber,
		Fiber,
		Fiber,
		FabricF<Fiber, Fiber>::type
	> f_nested2(std::make_tuple(
		// Terminal
		std::make_tuple(std::make_tuple(), 0),
		// Other fibers
		std::make_tuple(1),
		std::make_tuple(2),
		std::make_tuple(3),
		std::make_tuple(4),
		std::make_tuple(std::make_tuple(1), std::make_tuple(2))
	));
	f_nested2.outer_call<"did_recv"_tag>(0, Buffer(5));

	// Nested fabric
	Fabric<
		Terminal,
		FabricF<Fiber, Fiber>::type,
		Fiber,
		Fiber,
		Fiber,
		Fiber
	> f_nested3(std::make_tuple(
		// Terminal
		std::make_tuple(std::make_tuple(), 0),
		// Other fibers
		std::make_tuple(std::make_tuple(1), std::make_tuple(2)),
		std::make_tuple(1),
		std::make_tuple(2),
		std::make_tuple(3),
		std::make_tuple(4)
	));
	f_nested3.outer_call<"did_recv"_tag>(0, Buffer(5));

	// Nested fabric
	Fabric<
		Terminal,
		Fiber,
		Fiber,
		FabricF<Fiber, Fiber>::type,
		Fiber,
		Fiber
	> f_nested4(std::make_tuple(
		// Terminal
		std::make_tuple(std::make_tuple(), 0),
		// Other fibers
		std::make_tuple(1),
		std::make_tuple(2),
		std::make_tuple(std::make_tuple(1), std::make_tuple(2)),
		std::make_tuple(3),
		std::make_tuple(4)
	));
	f_nested4.outer_call<"did_recv"_tag>(0, Buffer(5));

	// Nested fabric
	Fabric<
		Terminal,
		Fiber,
		FabricF<Fiber, Fiber>::type,
		Fiber,
		FabricF<Fiber, Fiber>::type,
		Fiber,
		FabricF<Fiber, Fiber>::type,
		Fiber
	> f_nested5(std::make_tuple(
		// Terminal
		std::make_tuple(std::make_tuple(), 0),
		// Other fibers
		std::make_tuple(1),
		std::make_tuple(std::make_tuple(1), std::make_tuple(2)),
		std::make_tuple(2),
		std::make_tuple(std::make_tuple(1), std::make_tuple(2)),
		std::make_tuple(3),
		std::make_tuple(std::make_tuple(1), std::make_tuple(2)),
		std::make_tuple(4)
	));
	f_nested5.outer_call<"did_recv"_tag>(0, Buffer(5));

	return 0;
}
