#include <marlin/core/fabric/Fabric.hpp>
#include <spdlog/spdlog.h>

using namespace marlin::core;


size_t operator "" _sz (unsigned long long x) {
  return x;
}

struct Empty {
	template<typename... Args>
	Empty(Args&&...) {}
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
	int did_recv(FabricType&& fabric, Buffer&& buf) {
		SPDLOG_INFO("Did recv: {}", idx);
		if constexpr (std::is_same_v<ExtFabric, Empty>) {
			return 0;
		} else {
			return fabric.did_recv(*this, std::move(buf));
		}
	}
};


int main() {
	// Simplest fabric
	Fabric<
		Fiber<Empty>,
		Fiber
	> f_simplest(std::make_tuple(
		// Fiber<Empty>
		std::make_tuple(std::make_tuple(), 0_sz),
		// Other fibers
		std::make_tuple(1_sz)
	));
	f_simplest.did_recv(0_sz, Buffer(5));

	// Multiple fibers fabric
	Fabric<
		Fiber<Empty>,
		Fiber,
		Fiber,
		Fiber,
		Fiber,
		Fiber
	> f_multiple(std::make_tuple(
		// Fiber<Empty>
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
		Fiber<Empty>,
		FabricF<Fiber, Fiber>::type
	> f_nested(std::make_tuple(
		// Fiber<Empty>
		std::make_tuple(std::make_tuple(), 0_sz),
		// Other fibers
		std::make_tuple(std::make_tuple(1_sz), std::make_tuple(2_sz))
	));
	f_nested.did_recv(0_sz, Buffer(5));

	// Nested fabric
	// !!! Does not compile
	//Fabric<
		//Fiber<Empty>,
		//Fiber,
		//Fiber,
		//Fiber,
		//Fiber,
		//FabricF<Fiber, Fiber>::type
	//> f_nested2(std::make_tuple(
		//// Fiber<Empty>
		//std::make_tuple(std::make_tuple(), 0_sz),
		//// Other fibers
		//std::make_tuple(1_sz),
		//std::make_tuple(2_sz),
		//std::make_tuple(3_sz),
		//std::make_tuple(4_sz),
		//std::make_tuple(std::make_tuple(1_sz), std::make_tuple(2_sz))
	//));
	//f_nested2.did_recv(0_sz, Buffer(5));

	// Nested fabric
	// !!! Does not compile
	//Fabric<
		//Fiber<Empty>,
		//FabricF<Fiber, Fiber>::type,
		//Fiber,
		//Fiber,
		//Fiber,
		//Fiber
	//> f_nested3(std::make_tuple(
		//// Fiber<Empty>
		//std::make_tuple(std::make_tuple(), 0_sz),
		//// Other fibers
		//std::make_tuple(std::make_tuple(1_sz), std::make_tuple(2_sz)),
		//std::make_tuple(1_sz),
		//std::make_tuple(2_sz),
		//std::make_tuple(3_sz),
		//std::make_tuple(4_sz)
	//));
	//f_nested3.did_recv(0_sz, Buffer(5));

	// Nested fabric
	Fabric<
		Fiber<Empty>,
		Fiber,
		Fiber,
		FabricF<Fiber, Fiber>::type,
		Fiber,
		Fiber
	> f_nested4(std::make_tuple(
		// Fiber<Empty>
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
		Fiber<Empty>,
		Fiber,
		FabricF<Fiber, Fiber>::type,
		Fiber,
		FabricF<Fiber, Fiber>::type,
		Fiber,
		FabricF<Fiber, Fiber>::type,
		Fiber
	> f_nested5(std::make_tuple(
		// Fiber<Empty>
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

