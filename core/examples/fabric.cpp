#include <marlin/core/fabric/Fabric.hpp>
#include <spdlog/spdlog.h>

using namespace marlin::core;

template<size_t idx = 0>
struct Fiber {
	static constexpr bool is_outer_open = true;
	static constexpr bool is_inner_open = true;

	using InnerMessageType = Buffer;
	using OuterMessageType = Buffer;

	template<typename FabricType>
	int did_recv(FabricType&& fabric, Buffer&& buf) {
		SPDLOG_INFO("Did recv: {}", idx);
		return fabric.did_recv(*this, std::move(buf));
	}
};

int main() {
	Fabric<Fiber<0>, Fabric<Fiber<1>, Fiber<2>>, Fiber<3>, Fiber<4>> f;
	// unique identity rule strikes again -_-
	SPDLOG_INFO("{}", sizeof(f));

	return f.did_recv(Fiber<0>(), Buffer(5));
}

