#ifndef MARLIN_SIMULATOR_NETWORK_NETWORKCONDITIONER_HPP
#define MARLIN_SIMULATOR_NETWORK_NETWORKCONDITIONER_HPP

#include "marlin/core/SocketAddress.hpp"

namespace marlin {
namespace simulator {

class NetworkConditioner {
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
	uint64_t,
	core::SocketAddress const&,
	core::SocketAddress const&,
	uint64_t
) {
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
