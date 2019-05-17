#ifndef MARLIN_STREAM_STREAMPACKET_HPP
#define MARLIN_STREAM_STREAMPACKET_HPP

#include <marlin/net/Packet.hpp>

#include <cstring>
#include <arpa/inet.h>

namespace marlin {
namespace stream {

// TODO: Low priority - Investigate cross-platform way.
// Doesn't work for obscure endian systems(neither big or little).
// std::endian in C++20 should hopefully get us a long way there.
#define htonll(x) (htons(1) == 1 ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) (ntohs(1) == 1 ? (x) : ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))

struct StreamPacket: public net::Packet {
	uint8_t version() const {
		// Protect against overflow
		if(size() < 1)
			return -1;
		return data()[0];
	}

	uint8_t message() const {
		// Protect against overflow
		if(size() < 2)
			return -1;
		return data()[1];
	}

	bool is_fin_set() const {
		return message() == 1;
	}

	uint16_t stream_id() const {
		return extract_uint16(2);
	}

	uint64_t packet_number() const {
		return extract_uint64(4);
	}

	uint64_t offset() const {
		return extract_uint64(12);
	}

	uint16_t length() const {
		return extract_uint16(20);
	}
};

} // namespace stream
} // namespace marlin

#endif // MARLIN_STREAM_STREAMPACKET_HPP
