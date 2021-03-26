#include <marlin/core/fabric/Fabric.hpp>


using namespace marlin::core;

struct Fiber {
	static constexpr bool is_outer_open = true;
	static constexpr bool is_inner_open = true;

	using InnerMessageType = int;
	using OuterMessageType = int;
};

int main() {
	Fabric<Fiber, Fiber, Fiber> f;
	(void)f;
	return 0;
}

