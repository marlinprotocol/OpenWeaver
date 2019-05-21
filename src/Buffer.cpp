#include "Buffer.hpp"
#include <arpa/inet.h>
#include <cstring>

// TODO: Low priority - Investigate cross-platform way.
// Doesn't work for obscure endian systems(neither big or little).
// std::endian in C++20 should hopefully get us a long way there.
#define htonll(x) (htons(1) == 1 ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) (ntohs(1) == 1 ? (x) : ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))

namespace marlin {
namespace net {

Buffer::Buffer(std::unique_ptr<char[]> &&_buf, size_t const size) :
buf(_buf.release()), capacity(size), start_index(0) {}

Buffer::Buffer(char *const _buf, size_t const size) :
buf(_buf), capacity(size), start_index(0) {}

Buffer::Buffer(Buffer &&b) :
buf(b.buf), capacity(b.capacity), start_index(b.start_index) {
	b.buf = nullptr;
}

Buffer &Buffer::operator=(Buffer &&b) {
	// Destroy old
	delete[] buf;

	// Assign from new
	buf = b.buf;
	b.buf = nullptr;
	capacity = b.capacity;
	start_index = b.start_index;

	return *this;
}

Buffer::~Buffer() {
	delete[] buf;
}

bool Buffer::cover(size_t const num) {
	// Bounds checking
	if (num >= capacity - start_index) // Condition specifically avoids overflow
		return false;

	start_index += num;
	return true;
}

bool Buffer::uncover(size_t const num) {
	// Bounds checking
	if (start_index < num)
		return false;

	start_index -= num;
	return true;
}

uint8_t Buffer::extract_uint8(size_t const pos) const {
	// Bounds checking
	if(size() < 1 || size() - 1 < pos)
		return -1;

	return data()[pos];
}

uint16_t Buffer::extract_uint16(size_t const pos) const {
	// Bounds checking
	if(size() < 2 || size() - 2 < pos)
		return -1;

	uint16_t res;
	std::memcpy(&res, data()+pos, 2);

	return ntohs(res);
}

uint32_t Buffer::extract_uint32(size_t const pos) const {
	// Bounds checking
	if(size() < 4 || size() - 4 < pos)
		return -1;

	uint32_t res;
	std::memcpy(&res, data()+pos, 4);

	return ntohl(res);
}

uint64_t Buffer::extract_uint64(size_t const pos) const {
	// Bounds checking
	if(size() < 8 || size() - 8 < pos)
		return -1;

	uint64_t res;
	std::memcpy(&res, data()+pos, 8);

	return ntohll(res);
}

} // namespace net
} // namespace marlin
