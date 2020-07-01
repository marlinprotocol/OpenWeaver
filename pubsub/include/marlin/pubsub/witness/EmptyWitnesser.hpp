#ifndef MARLIN_PUBSUB_WITNESS_EMPTYWITNESSER_HPP
#define MARLIN_PUBSUB_WITNESS_EMPTYWITNESSER_HPP

#include <stdint.h>
#include <marlin/core/Buffer.hpp>


namespace marlin {
namespace pubsub {

struct EmptyWitnesser {
	template<typename HeaderType>
	constexpr uint64_t witness_size(
		HeaderType
	) {
		return 0;
	}

	template<typename HeaderType>
	constexpr int witness(
		HeaderType,
		core::Buffer&,
		uint64_t = 0
	) {
		return 0;
	}

	constexpr std::optional<uint64_t> parse_size(core::Buffer&, uint64_t = 0) {
		return 0;
	}

	template<typename HeaderType, typename... Params>
	constexpr bool contains(HeaderType, Params&&...) {
		return false;
	}
};

} // namespace pubsub
} // namespace marlin

#endif // MARLIN_PUBSUB_WITNESS_EMPTYWITNESSER_HPP
