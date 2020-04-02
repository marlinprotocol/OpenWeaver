#ifndef MARLIN_PUBSUB_ATTESTATION_EMPTYATTESTER_HPP
#define MARLIN_PUBSUB_ATTESTATION_EMPTYATTESTER_HPP

#include <stdint.h>
#include <marlin/net/Buffer.hpp>


namespace marlin {
namespace pubsub {

struct EmptyAttester {
	template<typename HeaderType>
	constexpr uint64_t attestation_size(
		uint64_t,
		uint16_t,
		uint8_t const*,
		uint64_t,
		HeaderType
	) {
		return 0;
	}

	template<typename HeaderType>
	constexpr int attest(
		uint64_t,
		uint16_t,
		uint8_t const*,
		uint64_t,
		HeaderType,
		net::Buffer&,
		uint64_t = 0
	) {
		return 0;
	}

	template<typename HeaderType>
	constexpr bool verify(
		uint64_t,
		uint16_t,
		uint8_t const*,
		uint64_t,
		HeaderType
	) {
		return true;
	}

	constexpr uint64_t parse_size(net::Buffer&, uint64_t = 0) {
		return 0;
	}
};

} // namespace pubsub
} // namespace marlin

#endif // MARLIN_PUBSUB_ATTESTATION_EMPTYATTESTER_HPP
