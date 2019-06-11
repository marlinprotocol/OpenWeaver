#include "Buffer.hpp"
#include "Endian.hpp"
#include <arpa/inet.h>
#include <cstring>

namespace marlin {
namespace net {

Buffer::Buffer(std::unique_ptr<char[]> &&_buf, size_t const size) :
buf(_buf.release()), capacity(size), start_index(0) {}

Buffer::Buffer(char *const _buf, size_t const size) :
buf(_buf), capacity(size), start_index(0) {}

Buffer::Buffer(Buffer &&b) :
buf(b.buf), capacity(b.capacity), start_index(b.start_index) {
	b.buf = nullptr;
	b.capacity = 0;
	b.start_index = 0;
}

Buffer &Buffer::operator=(Buffer &&b) {
	// Destroy old
	delete[] buf;

	// Assign from new
	buf = b.buf;
	capacity = b.capacity;
	start_index = b.start_index;

	b.buf = nullptr;
	b.capacity = 0;
	b.start_index = 0;

	return *this;
}

Buffer::~Buffer() {
	delete[] buf;
}

bool Buffer::cover(size_t const num) {
	// Bounds checking
	if (num > capacity - start_index) // Condition specifically avoids overflow
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

uint8_t Buffer::read_uint8(size_t const pos) const {
	// Bounds checking
	if(size() < 1 || size() - 1 < pos)
		return -1;

	return data()[pos];
}

uint16_t Buffer::read_uint16(size_t const pos) const {
	// Bounds checking
	if(size() < 2 || size() - 2 < pos)
		return -1;

	uint16_t res;
	std::memcpy(&res, data()+pos, 2);

	return res;
}

uint32_t Buffer::read_uint32(size_t const pos) const {
	// Bounds checking
	if(size() < 4 || size() - 4 < pos)
		return -1;

	uint32_t res;
	std::memcpy(&res, data()+pos, 4);

	return res;
}

uint64_t Buffer::read_uint64(size_t const pos) const {
	// Bounds checking
	if(size() < 8 || size() - 8 < pos)
		return -1;

	uint64_t res;
	std::memcpy(&res, data()+pos, 8);

	return res;
}

bool Buffer::write_uint8(size_t const pos, uint8_t const num) {
	// Bounds checking
	if(size() < 1 || size() - 1 < pos)
		return false;

	data()[pos] = num;

	return true;
}

bool Buffer::write_uint16(size_t const pos, uint16_t const num) {
	// Bounds checking
	if(size() < 2 || size() - 2 < pos)
		return false;

	std::memcpy(data()+pos, &num, 2);

	return true;
}

bool Buffer::write_uint32(size_t const pos, uint32_t const num) {
	// Bounds checking
	if(size() < 4 || size() - 4 < pos)
		return false;

	std::memcpy(data()+pos, &num, 4);

	return true;
}

bool Buffer::write_uint64(size_t const pos, uint64_t const num) {
	// Bounds checking
	if(size() < 8 || size() - 8 < pos)
		return false;

	std::memcpy(data()+pos, &num, 8);

	return true;
}

#if MARLIN_NET_ENDIANNESS == MARLIN_NET_BIG_ENDIAN
#define htonll(x) (x)
#define ntohll(x) (x)

uint16_t Buffer::read_uint16_le(size_t const pos) const {
	return htons(read_uint16(pos));
}

uint32_t Buffer::read_uint32_le(size_t const pos) const {
	return htonl(read_uint32(pos));
}

uint64_t Buffer::read_uint64_le(size_t const pos) const {
	return htonll(read_uint64(pos));
}

uint16_t Buffer::read_uint16_be(size_t const pos) const {
	return read_uint16(pos);
}

uint32_t Buffer::read_uint32_be(size_t const pos) const {
	return read_uint32(pos);
}

uint64_t Buffer::read_uint64_be(size_t const pos) const {
	return read_uint64(pos);
}

bool Buffer::write_uint16_le(size_t const pos, uint16_t const num) {
	return write_uint16(pos, htons(num));
}

bool Buffer::write_uint32_le(size_t const pos, uint32_t const num) {
	return write_uint32(pos, htonl(num));
}

bool Buffer::write_uint64_le(size_t const pos, uint64_t const num) {
	return write_uint64(pos, htonll(num));
}

bool Buffer::write_uint16_be(size_t const pos, uint16_t const num) {
	return write_uint16(pos, num);
}

bool Buffer::write_uint32_be(size_t const pos, uint32_t const num) {
	return write_uint32(pos, num);
}

bool Buffer::write_uint64_be(size_t const pos, uint64_t const num) {
	return write_uint64(pos, num);
}

#elif MARLIN_NET_ENDIANNESS == MARLIN_NET_LITTLE_ENDIAN
#define htonll(x) ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32)
#define ntohll(x) ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32)

uint16_t Buffer::read_uint16_be(size_t const pos) const {
	return htons(read_uint16(pos));
}

uint32_t Buffer::read_uint32_be(size_t const pos) const {
	return htonl(read_uint32(pos));
}

uint64_t Buffer::read_uint64_be(size_t const pos) const {
	return htonll(read_uint64(pos));
}

uint16_t Buffer::read_uint16_le(size_t const pos) const {
	return read_uint16(pos);
}

uint32_t Buffer::read_uint32_le(size_t const pos) const {
	return read_uint32(pos);
}

uint64_t Buffer::read_uint64_le(size_t const pos) const {
	return read_uint64(pos);
}

bool Buffer::write_uint16_be(size_t const pos, uint16_t const num) {
	return write_uint16(pos, htons(num));
}

bool Buffer::write_uint32_be(size_t const pos, uint32_t const num) {
	return write_uint32(pos, htonl(num));
}

bool Buffer::write_uint64_be(size_t const pos, uint64_t const num) {
	return write_uint64(pos, htonll(num));
}

bool Buffer::write_uint16_le(size_t const pos, uint16_t const num) {
	return write_uint16(pos, num);
}

bool Buffer::write_uint32_le(size_t const pos, uint32_t const num) {
	return write_uint32(pos, num);
}

bool Buffer::write_uint64_le(size_t const pos, uint64_t const num) {
	return write_uint64(pos, num);
}

#endif

} // namespace net
} // namespace marlin
