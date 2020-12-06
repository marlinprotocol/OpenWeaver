#ifndef MARLIN_PUBSUB_ATTESTATION_LPFATTESTER_HPP
#define MARLIN_PUBSUB_ATTESTATION_LPFATTESTER_HPP

#include <stdint.h>
#include <marlin/core/WeakBuffer.hpp>
#include <ctime>
#include <optional>


namespace marlin {
namespace pubsub {

struct LpfAttester {
	template<typename HeaderType>
	constexpr uint64_t attestation_size(
		uint64_t,
		uint16_t,
		uint8_t const*,
		uint64_t,
		HeaderType prev_header
	) {
		return prev_header.attestation_size == 0 ? 2 : prev_header.attestation_size;
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

		out.write_uint16_le_unsafe(offset, 0);

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
		auto len = buf.read_uint16_le(offset);
		return len;
	}
};

} // namespace pubsub
} // namespace marlin

#endif // MARLIN_PUBSUB_ATTESTATION_LPFATTESTER_HPP
