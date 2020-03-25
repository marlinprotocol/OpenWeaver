#ifndef MARLIN_PUBSUB_WITNESS_BASE_HPP
#define MARLIN_PUBSUB_WITNESS_BASE_HPP

#include <stdint.h>


namespace marlin {
namespace pubsub {

template<typename WitnesserType, bool b>
struct WitnesserBase {};

template<typename WitnesserType>
struct WitnesserBase<WitnesserType, true> {
	WitnesserType witnesser;
};

template<bool>
struct WitnessHeader {};

template<>
struct WitnessHeader<true> {
	char const* witness_data = nullptr;
	uint64_t witness_size = 0;
};

} // namespace pubsub
} // namespace marlin

#endif // MARLIN_PUBSUB_WITNESS_BASE_HPP
