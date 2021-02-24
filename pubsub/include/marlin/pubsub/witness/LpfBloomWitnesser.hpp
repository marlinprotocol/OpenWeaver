#ifndef MARLIN_PUBSUB_WITNESS_LPFBLOOMWITNESSER_HPP
#define MARLIN_PUBSUB_WITNESS_LPFBLOOMWITNESSER_HPP

#include <stdint.h>
#include <marlin/core/Buffer.hpp>


namespace marlin {
namespace pubsub {

struct LpfBloomWitnesser {
	using KeyType = uint8_t const*;
	KeyType public_key;

	LpfBloomWitnesser(KeyType public_key) : public_key(public_key) {}

	template<typename HeaderType>
	constexpr uint64_t witness_size(
		HeaderType
	) {
		return 34;
	}

	void set_bit(uint8_t* bloom, uint8_t idx) {
		bloom[idx / 8] |= (1 << (idx%8));
	}

	bool test_bit(uint8_t const* bloom, uint8_t idx) {
		return (bloom[idx / 8] & (1 << (idx%8))) != 0;
	}

	template<typename HeaderType>
	bool contains(
		HeaderType prev_witness_header,
		KeyType public_key
	) {
		bool found = true;
		for(uint i = 0; i < 8; i++) {
			found = found && test_bit(prev_witness_header.witness_data + 2, public_key[i]);
		}

		return found;
	}

	template<typename HeaderType>
	int witness(
		HeaderType prev_witness_header,
		core::Buffer& out,
		uint64_t offset = 0
	) {
		SPDLOG_DEBUG("LpfBloomWitnesser: witness");
		if(prev_witness_header.witness_data == nullptr) {
			// Set length
			out.write_uint16_le_unsafe(offset, 34);
			// Zero out bloom filter
			std::memset(out.data() + offset + 2, 0, 32);
		} else {
			// Copy bloom filter as is
			out.write_unsafe(offset, prev_witness_header.witness_data, 34);
		}

		// Set own bits in bloom filter
		for(uint i = 0; i < 8; i++) {
			set_bit(out.data() + offset + 2, public_key[i]);
		}
		return 0;
	}

	std::optional<uint64_t> parse_size(core::Buffer& buf, uint64_t offset = 0) {
		if(buf.read_uint16_le(offset) == 34) {
			return 34;
		}
		return std::nullopt;
	}
};

} // namespace pubsub
} // namespace marlin

#endif // MARLIN_PUBSUB_WITNESS_LPFBLOOMWITNESSER_HPP
