#include <marlin/core/fabric/Fabric.hpp>
#include <spdlog/spdlog.h>

using namespace marlin::core;

struct Fiber {
	static constexpr bool is_outer_open = true;
	static constexpr bool is_inner_open = true;

	using InnerMessageType = Buffer;
	using OuterMessageType = Buffer;
};

int main() {
	Fabric<Fiber, Fabric<Fiber, Fiber>, Fiber, Fiber> f;
	// unique identity rule strikes again -_-
	SPDLOG_INFO("{}", sizeof(f));
	return 0;
}

