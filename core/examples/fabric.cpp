#include <marlin/core/fabric/Fabric.hpp>
#include <spdlog/spdlog.h>

using namespace marlin::core;


size_t operator "" _sz (unsigned long long x) {
  return x;
}

struct Terminal {
	static constexpr bool is_outer_open = false;
	static constexpr bool is_inner_open = false;

	using InnerMessageType = Buffer;
	using OuterMessageType = Buffer;

	template<typename... Args>
	Terminal(Args&&...) {}

	template<typename FabricType>
	int did_recv(FabricType&&, Buffer&&) {
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
	size_t idx = 0;

	template<typename ExtTupleType>
	Fiber(std::tuple<ExtTupleType, size_t>&& init_tuple) :
		ext_fabric(std::get<0>(init_tuple)),
		idx(std::get<1>(init_tuple)) {}

	template<typename FabricType>
	int did_recv(FabricType&&, Buffer&& buf) {
		SPDLOG_INFO("Did recv: {}", idx);
		return ext_fabric.did_recv(*this, std::move(buf));
	}
};


int main() {
	// Simplest fabric
	Fabric<
		Terminal,
		Fiber
	> f_simplest(std::make_tuple(
		// Terminal
		std::make_tuple(std::make_tuple(), 0_sz),
		// Other fibers
		std::make_tuple(1_sz)
	));
	f_simplest.did_recv(0_sz, Buffer(5));

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
		std::make_tuple(std::make_tuple(), 0_sz),
		// Other fibers
		std::make_tuple(1_sz),
		std::make_tuple(2_sz),
		std::make_tuple(3_sz),
		std::make_tuple(4_sz),
		std::make_tuple(5_sz)
	));
	f_multiple.did_recv(0_sz, Buffer(5));

	// Nested fabric
	Fabric<
		Terminal,
		FabricF<Fiber, Fiber>::type
	> f_nested(std::make_tuple(
		// Terminal
		std::make_tuple(std::make_tuple(), 0_sz),
		// Other fibers
		std::make_tuple(std::make_tuple(1_sz), std::make_tuple(2_sz))
	));
	f_nested.did_recv(0_sz, Buffer(5));

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
		std::make_tuple(std::make_tuple(), 0_sz),
		// Other fibers
		std::make_tuple(1_sz),
		std::make_tuple(2_sz),
		std::make_tuple(3_sz),
		std::make_tuple(4_sz),
		std::make_tuple(std::make_tuple(1_sz), std::make_tuple(2_sz))
	));
	f_nested2.did_recv(0_sz, Buffer(5));

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
		std::make_tuple(std::make_tuple(), 0_sz),
		// Other fibers
		std::make_tuple(std::make_tuple(1_sz), std::make_tuple(2_sz)),
		std::make_tuple(1_sz),
		std::make_tuple(2_sz),
		std::make_tuple(3_sz),
		std::make_tuple(4_sz)
	));
	f_nested3.did_recv(0_sz, Buffer(5));

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
		std::make_tuple(std::make_tuple(), 0_sz),
		// Other fibers
		std::make_tuple(1_sz),
		std::make_tuple(2_sz),
		std::make_tuple(std::make_tuple(1_sz), std::make_tuple(2_sz)),
		std::make_tuple(3_sz),
		std::make_tuple(4_sz)
	));
	f_nested4.did_recv(0_sz, Buffer(5));

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
		std::make_tuple(std::make_tuple(), 0_sz),
		// Other fibers
		std::make_tuple(1_sz),
		std::make_tuple(std::make_tuple(1_sz), std::make_tuple(2_sz)),
		std::make_tuple(2_sz),
		std::make_tuple(std::make_tuple(1_sz), std::make_tuple(2_sz)),
		std::make_tuple(3_sz),
		std::make_tuple(std::make_tuple(1_sz), std::make_tuple(2_sz)),
		std::make_tuple(4_sz)
	));
	f_nested5.did_recv(0_sz, Buffer(5));

	return 0;
}

