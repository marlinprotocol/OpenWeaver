#ifndef MARLIN_PUBSUB_ATTESTATION_BASE_HPP
#define MARLIN_PUBSUB_ATTESTATION_BASE_HPP

#include <stdint.h>


namespace marlin {
namespace pubsub {

template<bool>
struct AttestationHeader {};

template<>
struct AttestationHeader<true> {
	char const* attestation_data = nullptr;
	uint64_t attestation_size = 0;
};

} // namespace pubsub
} // namespace marlin

#endif // MARLIN_PUBSUB_ATTESTATION_BASE_HPP
