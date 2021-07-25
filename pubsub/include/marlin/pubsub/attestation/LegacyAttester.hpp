#ifndef MARLIN_PUBSUB_ATTESTATION_LEGACYATTESTER_HPP
#define MARLIN_PUBSUB_ATTESTATION_LEGACYATTESTER_HPP

#include <stdint.h>
#include <marlin/core/WeakBuffer.hpp>
#include <marlin/core/Buffer.hpp>
#include <ctime>
#include <optional>

#include <secp256k1_recovery.h>
#include <cryptopp/keccak.h>
#include <cryptopp/osrng.h>


#include <spdlog/spdlog.h>


namespace marlin {
namespace pubsub {

struct LegacyAttester {
	template<typename HeaderType>
	constexpr uint64_t attestation_size(
		uint64_t,
		uint16_t,
		uint8_t const*,
		uint64_t,
		HeaderType prev_header
	) {
		return prev_header.attestation_size == 67 ? 67 : 0;
	}

	template<typename HeaderType>
	int attest(
		uint64_t,
		uint16_t,
		uint8_t const*,
		uint64_t,
		HeaderType prev_header,
		core::Buffer& out,
		uint64_t offset = 0
	) {
		if(prev_header.attestation_size != 0) {
			// FIXME: should probably add _unsafe to function
			out.write_unsafe(offset, prev_header.attestation_data, prev_header.attestation_size);
			return 1;
		}

		return 0;
	}

	template<typename HeaderType>
	bool verify(
		uint64_t,
		uint16_t,
		uint8_t const*,
		uint64_t,
		HeaderType
	) {
		return true;
	}

	std::optional<uint64_t> parse_size(core::Buffer& buf, uint64_t offset = 0) {
		if(buf.read_uint16_le(offset) == 67) {
			return 67;
		}
		return 0;
	}
};

} // namespace pubsub
} // namespace marlin

#endif // MARLIN_PUBSUB_ATTESTATION_LEGACYATTESTER_HPP
