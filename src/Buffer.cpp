#include <marlin/net/Buffer.hpp>
#include <marlin/net/Endian.hpp>
#include <cstring>

namespace marlin {
namespace net {

Buffer::Buffer(std::unique_ptr<char[]> &&_buf, size_t const size) :
buf(_buf.release()), capacity(size), start_index(0), end_index(size) {}

Buffer::Buffer(char *const _buf, size_t const size) :
buf(_buf), capacity(size), start_index(0), end_index(size) {}

Buffer::Buffer(Buffer &&b) :
buf(b.buf), capacity(b.capacity), start_index(b.start_index), end_index(b.capacity) {
	b.buf = nullptr;
	b.capacity = 0;
	b.start_index = 0;
	b.end_index = 0;
}

Buffer &Buffer::operator=(Buffer &&b) {
	// Destroy old
	delete[] buf;

	// Assign from new
	buf = b.buf;
	capacity = b.capacity;
	start_index = b.start_index;
	end_index = b.end_index;

	b.buf = nullptr;
	b.capacity = 0;
	b.start_index = 0;
	b.end_index = 0;

	return *this;
}

Buffer::~Buffer() {
	delete[] buf;
}

bool Buffer::cover(size_t const num) {
	// Bounds checking
	if (num > size())
		return false;

	cover_unsafe(num);

	return true;
}

void Buffer::cover_unsafe(size_t const num) {
	start_index += num;
}

bool Buffer::uncover(size_t const num) {
	// Bounds checking
	if (start_index < num)
		return false;

	uncover_unsafe(num);

	return true;
}

void Buffer::uncover_unsafe(size_t const num) {
	start_index -= num;
}

bool Buffer::truncate(size_t const num) {
	// Bounds checking
	if (num > size())
		return false;

	truncate_unsafe(num);

	return true;
}

void Buffer::truncate_unsafe(size_t const num) {
	end_index -= num;
}

bool Buffer::expand(size_t const num) {
	// Bounds checking
	if (capacity - end_index < num)
		return false;

	expand_unsafe(num);

	return true;
}

void Buffer::expand_unsafe(size_t const num) {
	end_index += num;
}

uint8_t Buffer::read_uint8_unsafe(size_t const pos) const {
	return data()[pos];
}

uint16_t Buffer::read_uint16_unsafe(size_t const pos) const {
	uint16_t res;
	std::memcpy(&res, data()+pos, 2);

	return res;
}

uint32_t Buffer::read_uint32_unsafe(size_t const pos) const {
	uint32_t res;
	std::memcpy(&res, data()+pos, 4);

	return res;
}

uint64_t Buffer::read_uint64_unsafe(size_t const pos) const {
	uint64_t res;
	std::memcpy(&res, data()+pos, 8);

	return res;
}

uint8_t Buffer::read_uint8(size_t const pos) const {
	// Bounds checking
	if(size() < 1 || size() - 1 < pos)
		return -1;

	return read_uint8_unsafe(pos);
}

uint16_t Buffer::read_uint16(size_t const pos) const {
	// Bounds checking
	if(size() < 2 || size() - 2 < pos)
		return -1;

	return read_uint16_unsafe(pos);
}

uint32_t Buffer::read_uint32(size_t const pos) const {
	// Bounds checking
	if(size() < 4 || size() - 4 < pos)
		return -1;

	return read_uint32_unsafe(pos);
}

uint64_t Buffer::read_uint64(size_t const pos) const {
	// Bounds checking
	if(size() < 8 || size() - 8 < pos)
		return -1;

	return read_uint64_unsafe(pos);
}

void Buffer::write_uint8_unsafe(size_t const pos, uint8_t const num) {
	data()[pos] = num;
}

void Buffer::write_uint16_unsafe(size_t const pos, uint16_t const num) {
	std::memcpy(data()+pos, &num, 2);
}

void Buffer::write_uint32_unsafe(size_t const pos, uint32_t const num) {
	std::memcpy(data()+pos, &num, 4);
}

void Buffer::write_uint64_unsafe(size_t const pos, uint64_t const num) {
	std::memcpy(data()+pos, &num, 8);
}

bool Buffer::write_uint8(size_t const pos, uint8_t const num) {
	// Bounds checking
	if(size() < 1 || size() - 1 < pos)
		return false;

	write_uint8_unsafe(pos, num);

	return true;
}

bool Buffer::write_uint16(size_t const pos, uint16_t const num) {
	// Bounds checking
	if(size() < 2 || size() - 2 < pos)
		return false;

	write_uint16_unsafe(pos, num);

	return true;
}

bool Buffer::write_uint32(size_t const pos, uint32_t const num) {
	// Bounds checking
	if(size() < 4 || size() - 4 < pos)
		return false;

	write_uint32_unsafe(pos, num);

	return true;
}

bool Buffer::write_uint64(size_t const pos, uint64_t const num) {
	// Bounds checking
	if(size() < 8 || size() - 8 < pos)
		return false;

	write_uint64_unsafe(pos, num);

	return true;
}

#if MARLIN_NET_ENDIANNESS == MARLIN_NET_BIG_ENDIAN

uint16_t Buffer::read_uint16_le(size_t const pos) const {
	return __builtin_bswap16(read_uint16(pos));
}

uint32_t Buffer::read_uint32_le(size_t const pos) const {
	return __builtin_bswap32(read_uint32(pos));
}

uint64_t Buffer::read_uint64_le(size_t const pos) const {
	return __builtin_bswap64(read_uint64(pos));
}

uint16_t Buffer::read_uint16_le_unsafe(size_t const pos) const {
	return __builtin_bswap16(read_uint16_unsafe(pos));
}

uint32_t Buffer::read_uint32_le_unsafe(size_t const pos) const {
	return __builtin_bswap32(read_uint32_unsafe(pos));
}

uint64_t Buffer::read_uint64_le_unsafe(size_t const pos) const {
	return __builtin_bswap64(read_uint64_unsafe(pos));
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

uint16_t Buffer::read_uint16_be_unsafe(size_t const pos) const {
	return read_uint16_unsafe(pos);
}

uint32_t Buffer::read_uint32_be_unsafe(size_t const pos) const {
	return read_uint32_unsafe(pos);
}

uint64_t Buffer::read_uint64_be_unsafe(size_t const pos) const {
	return read_uint64_unsafe(pos);
}

bool Buffer::write_uint16_le(size_t const pos, uint16_t const num) {
	return write_uint16(pos, __builtin_bswap16(num));
}

bool Buffer::write_uint32_le(size_t const pos, uint32_t const num) {
	return write_uint32(pos, __builtin_bswap32(num));
}

bool Buffer::write_uint64_le(size_t const pos, uint64_t const num) {
	return write_uint64(pos, __builtin_bswap64(num));
}

void Buffer::write_uint16_le_unsafe(size_t const pos, uint16_t const num) {
	return write_uint16_unsafe(pos, __builtin_bswap16(num));
}

void Buffer::write_uint32_le_unsafe(size_t const pos, uint32_t const num) {
	return write_uint32_unsafe(pos, __builtin_bswap32(num));
}

void Buffer::write_uint64_le_unsafe(size_t const pos, uint64_t const num) {
	return write_uint64_unsafe(pos, __builtin_bswap64(num));
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

void Buffer::write_uint16_be_unsafe(size_t const pos, uint16_t const num) {
	return write_uint16_unsafe(pos, num);
}

void Buffer::write_uint32_be_unsafe(size_t const pos, uint32_t const num) {
	return write_uint32_unsafe(pos, num);
}

void Buffer::write_uint64_be_unsafe(size_t const pos, uint64_t const num) {
	return write_uint64_unsafe(pos, num);
}

#elif MARLIN_NET_ENDIANNESS == MARLIN_NET_LITTLE_ENDIAN

uint16_t Buffer::read_uint16_be(size_t const pos) const {
	return __builtin_bswap16(read_uint16(pos));
}

uint32_t Buffer::read_uint32_be(size_t const pos) const {
	return __builtin_bswap32(read_uint32(pos));
}

uint64_t Buffer::read_uint64_be(size_t const pos) const {
	return __builtin_bswap64(read_uint64(pos));
}

uint16_t Buffer::read_uint16_be_unsafe(size_t const pos) const {
	return __builtin_bswap16(read_uint16_unsafe(pos));
}

uint32_t Buffer::read_uint32_be_unsafe(size_t const pos) const {
	return __builtin_bswap32(read_uint32_unsafe(pos));
}

uint64_t Buffer::read_uint64_be_unsafe(size_t const pos) const {
	return __builtin_bswap64(read_uint64_unsafe(pos));
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

uint16_t Buffer::read_uint16_le_unsafe(size_t const pos) const {
	return read_uint16_unsafe(pos);
}

uint32_t Buffer::read_uint32_le_unsafe(size_t const pos) const {
	return read_uint32_unsafe(pos);
}

uint64_t Buffer::read_uint64_le_unsafe(size_t const pos) const {
	return read_uint64_unsafe(pos);
}

bool Buffer::write_uint16_be(size_t const pos, uint16_t const num) {
	return write_uint16(pos, __builtin_bswap16(num));
}

bool Buffer::write_uint32_be(size_t const pos, uint32_t const num) {
	return write_uint32(pos, __builtin_bswap32(num));
}

bool Buffer::write_uint64_be(size_t const pos, uint64_t const num) {
	return write_uint64(pos, __builtin_bswap64(num));
}

void Buffer::write_uint16_be_unsafe(size_t const pos, uint16_t const num) {
	return write_uint16_unsafe(pos, __builtin_bswap16(num));
}

void Buffer::write_uint32_be_unsafe(size_t const pos, uint32_t const num) {
	return write_uint32_unsafe(pos, __builtin_bswap32(num));
}

void Buffer::write_uint64_be_unsafe(size_t const pos, uint64_t const num) {
	return write_uint64_unsafe(pos, __builtin_bswap64(num));
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

void Buffer::write_uint16_le_unsafe(size_t const pos, uint16_t const num) {
	return write_uint16_unsafe(pos, num);
}

void Buffer::write_uint32_le_unsafe(size_t const pos, uint32_t const num) {
	return write_uint32_unsafe(pos, num);
}

void Buffer::write_uint64_le_unsafe(size_t const pos, uint64_t const num) {
	return write_uint64_unsafe(pos, num);
}

#endif

} // namespace net
} // namespace marlin
