#ifndef MARLIN_PUBSUB_WITNESS_LPFWITNESSER_HPP
#define MARLIN_PUBSUB_WITNESS_LPFWITNESSER_HPP

#include <stdint.h>
#include <marlin/core/Buffer.hpp>


namespace marlin {
namespace pubsub {

struct LpfWitnesser {
	using KeyType = uint8_t const*;

	template<typename HeaderType>
	constexpr uint64_t witness_size(
		HeaderType prev_witness_header
	) {
		return prev_witness_header.witness_size == 0 ? 2 : (prev_witness_header.witness_size);
	}

	template<typename HeaderType>
	bool contains(
		HeaderType,
		KeyType
	) {
		return false;
	}

	template<typename HeaderType>
	int witness(
		HeaderType prev_witness_header,
		core::Buffer& out,
		uint64_t offset = 0
	) {
		if(prev_witness_header.witness_size != 0) {
			// FIXME: should probably add _unsafe to function
			out.write_unsafe(offset, prev_witness_header.witness_data, prev_witness_header.witness_size);
			return 1;
		}

		out.write_uint16_le_unsafe(offset, 0);

		return 0;
	}

	std::optional<uint64_t> parse_size(core::Buffer& in, uint64_t offset = 0) {
		auto res = in.read_uint16_le(offset);
		SPDLOG_DEBUG("LpfWitnesser: parse size: {}", res.value_or(-1));
		return res;
	}
};

} // namespace pubsub
} // namespace marlin

#endif // MARLIN_PUBSUB_WITNESS_LPFWITNESSER_HPP

