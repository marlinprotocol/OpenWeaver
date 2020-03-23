#ifndef MARLIN_PUBSUB_WITNESS_BASE_HPP
#define MARLIN_PUBSUB_WITNESS_BASE_HPP


namespace marlin {
namespace pubsub {

template<typename WitnesserType, bool b>
struct WitnesserBase {};

template<typename WitnesserType>
struct WitnesserBase<WitnesserType, true> {
	WitnesserType witnesser;
};

} // namespace pubsub
} // namespace marlin

#endif // MARLIN_PUBSUB_WITNESS_BASE_HPP
