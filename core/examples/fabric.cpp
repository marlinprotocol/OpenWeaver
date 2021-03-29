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
	Fabric<
		Fiber<Empty>,
		Fiber,
		Fiber,
		FabricF<Fiber, Fiber>::type,
		Fiber,
		Fiber
	> f(std::make_tuple(
		// Fiber<Empty>
		std::make_tuple(std::make_tuple(), 0_sz),
		// Other fibers
		std::make_tuple(5_sz),
		std::make_tuple(6_sz),
		std::make_tuple(std::make_tuple(1_sz), std::make_tuple(2_sz)),
		std::make_tuple(3_sz),
		std::make_tuple(4_sz)
	));
	// unique identity rule strikes again -_-
	SPDLOG_INFO("{}", sizeof(f));

	return f.did_recv(0_sz, Buffer(5));
}

