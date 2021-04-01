#ifndef MARLIN_SIMULATOR_NETWORK_NETWORKCONDITIONER_HPP
#define MARLIN_SIMULATOR_NETWORK_NETWORKCONDITIONER_HPP

#include "marlin/core/SocketAddress.hpp"
#include <marlin/utils/DropRanges.hpp>

namespace marlin {
namespace simulator {

class NetworkConditioner {
private:
	// utils::PacketDropRanges drop;

public:
	uint64_t count = 0;
	bool should_drop(
		uint64_t in_tick,
		core::SocketAddress const& src,
		core::SocketAddress const& dst,
		uint64_t size
	);

	std::function<bool()> drop_pattern; 

	uint64_t get_out_tick(
		uint64_t in_tick,
		core::SocketAddress const& src,
		core::SocketAddress const& dst,
		uint64_t size
	);
};


// Impl

bool NetworkConditioner::should_drop(
	uint64_t in_tick [[maybe_unused]],
	core::SocketAddress const& src [[maybe_unused]],
	core::SocketAddress const& dst [[maybe_unused]],
	uint64_t size [[maybe_unused]]
) {
	MARLIN_LOG_DEBUG("Tick {}, src {}, dst {}, size {}", in_tick, src.to_string(), dst.to_string(), size);
	return drop_pattern();
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
