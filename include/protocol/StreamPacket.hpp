#ifndef STREAM_STREAM_PACKET_HPP
#define STREAM_STREAM_PACKET_HPP

#include <cstring>
#include <arpa/inet.h>

namespace stream {

// TODO: Investigate cross-platform way.
// Doesn't work for obscure endian systems(neither big or little).
// std::endian in C++20 should hopefully get us a long way there.
#define htonll(x) (htons(1) == 1 ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) (ntohs(1) == 1 ? (x) : ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))

struct StreamPacket: public fiber::Packet {
	uint8_t version() const {
		// Protect against overflow
		if(size() < 2)
			return -1;
		return data()[1];
	}

	uint8_t message() const {
		// Protect against overflow
		if(size() < 3)
			return -1;
		return data()[2];
	}

	uint64_t stream_id() const {
		// Protect against overflow
		if(size() < 4)
			return -1;
		return data()[3];
	}

	uint64_t packet_number() const {
		// Protect against overflow
		if(size() < 12)
			return -1;
		uint64_t ret;  // memcpy instead of typecast to avoid alignment issues
		std::memcpy(&ret, data()+4, 8);
		return ntohll(ret);
	}

	uint64_t offset() const {
		// Protect against overflow
		if(size() < 20)
			return -1;
		uint64_t ret;  // memcpy instead of typecast to avoid alignment issues
		std::memcpy(&ret, data()+12, 8);
		return ntohll(ret);
	}

	uint16_t length() const {
		// Protect against overflow
		if(size() < 22)
			return -1;
		uint16_t ret;  // memcpy instead of typecast to avoid alignment issues
		std::memcpy(&ret, data()+20, 2);
		return ntohs(ret);
	}
};

} // namespace stream

#endif // STREAM_STREAM_PACKET_HPP
