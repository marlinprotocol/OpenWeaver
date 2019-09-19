#ifndef MARLIN_STREAM_STREAMPACKET_HPP
#define MARLIN_STREAM_STREAMPACKET_HPP

#include <marlin/net/Buffer.hpp>

#include <cstring>
#include <arpa/inet.h>

namespace marlin {
namespace stream {

struct StreamPacket: public net::Buffer {
	uint8_t version() const {
		return read_uint8(0);
	}

	uint8_t message() const {
		return read_uint8(1);
	}

	bool is_fin_set() const {
		return message() == 1;
	}

	uint16_t stream_id() const {
		return read_uint16_be(10);
	}

	uint64_t packet_number() const {
		return read_uint64_be(12);
	}

	uint64_t offset() const {
		return read_uint64_be(20);
	}

	uint16_t length() const {
		return read_uint16_be(28);
	}
};

} // namespace stream
} // namespace marlin

#endif // MARLIN_STREAM_STREAMPACKET_HPP
