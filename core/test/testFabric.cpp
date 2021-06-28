#include <gtest/gtest.h>
#include "marlin/core/fabric/Fabric.hpp"

#include <cstring>

using namespace marlin::core;

// namespace marlin {
// namespace core {

// }
// }

struct Terminal {
	static constexpr bool is_outer_open = false;
	static constexpr bool is_inner_open = false;

	using InnerMessageType = Buffer;
	using OuterMessageType = Buffer;

	template<typename... Args>
	Terminal(Args&&...) {}

	template<typename FiberType>
	int did_recv(FiberType&, Buffer&&) {
		// SPDLOG_INFO("Did recv: Terminal");
		return 0;
	}

	template<typename FiberType>
	int did_recv(std::vector <int> &indices, FiberType&, Buffer&&) {
		indices.push_back(-1);
		// SPDLOG_INFO("Did recv: Terminal");
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

	int did_recv(Buffer&& buf) {
		// SPDLOG_INFO("Did recv: {}", idx);
		return ext_fabric.did_recv(*this, std::move(buf));
	}

	int did_recv(std::vector <int> &indices, Buffer&& buf) {
		// SPDLOG_INFO("Did recv: {}", idx);
		indices.push_back(idx);
		return ext_fabric.did_recv(indices, *this, std::move(buf));
	}
};
