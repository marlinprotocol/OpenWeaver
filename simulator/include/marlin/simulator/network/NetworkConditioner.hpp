#ifndef MARLIN_SIMULATOR_NETWORK_NETWORKCONDITIONER_HPP
#define MARLIN_SIMULATOR_NETWORK_NETWORKCONDITIONER_HPP

#include "marlin/core/SocketAddress.hpp"

namespace marlin {
namespace simulator {

class NetworkConditioner {
	public:
		int count = 0;
public:
	bool should_drop(
		uint64_t in_tick,
		core::SocketAddress const& src,
		core::SocketAddress const& dst,
		uint64_t size
	);

	uint64_t get_out_tick(
		uint64_t in_tick,
		core::SocketAddress const& src,
		core::SocketAddress const& dst,
		uint64_t size
	);
};


// Impl

bool NetworkConditioner::should_drop(
	uint64_t in_tick,
	core::SocketAddress const& src,
	core::SocketAddress const& dst,
	uint64_t size
) {
	size++;
	SPDLOG_DEBUG("{}, {}, {}",in_tick,src.to_string(), dst.to_string());
	this->count++;
	if(count>20 && count<25)
		return true;
	return false;
}

uint64_t NetworkConditioner::get_out_tick(
	uint64_t in_tick,
	core::SocketAddress const&,
	core::SocketAddress const&,
	uint64_t
) {
	return in_tick + 1;
}

} // namespace simulator
} // namespace marlin

#endif // MARLIN_SIMULATOR_NETWORK_NETWORKCONDITIONER_HPP
