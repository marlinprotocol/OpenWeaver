#include "marlin/core/WeakBuffer.hpp"
#include "marlin/core/Endian.hpp"
#include <cstring>
#include <cassert>
#include <algorithm>

namespace marlin {
namespace core {

WeakBuffer::WeakBuffer(uint8_t *buf, size_t size) :
buf(buf), capacity(size), start_index(0), end_index(size) {}

bool WeakBuffer::cover(size_t num) {
	// Bounds checking
	if (num > size())
		return false;

	cover_unsafe(num);

	return true;
}

WeakBuffer& WeakBuffer::cover_unsafe(size_t num) & {
	assert(num <= size());

	start_index += num;

	return *this;
}

WeakBuffer&& WeakBuffer::cover_unsafe(size_t num) && {
	return std::move(cover_unsafe(num));
}

bool WeakBuffer::uncover(size_t num) {
	// Bounds checking
	if (start_index < num)
		return false;

	uncover_unsafe(num);

	return true;
}

WeakBuffer& WeakBuffer::uncover_unsafe(size_t num) & {
	assert(start_index >= num);

	start_index -= num;

	return *this;
}

WeakBuffer&& WeakBuffer::uncover_unsafe(size_t num) && {
	return std::move(uncover_unsafe(num));
}

bool WeakBuffer::truncate(size_t num) {
	// Bounds checking
	if (num > size())
		return false;

	truncate_unsafe(num);

	return true;
}

WeakBuffer& WeakBuffer::truncate_unsafe(size_t num) & {
	assert(num <= size());

	end_index -= num;

	return *this;
}

WeakBuffer&& WeakBuffer::truncate_unsafe(size_t num) && {
	return std::move(truncate_unsafe(num));
}

bool WeakBuffer::expand(size_t num) {
	// Bounds checking
	if (capacity - end_index < num)
		return false;

	expand_unsafe(num);

	return true;
}

WeakBuffer& WeakBuffer::expand_unsafe(size_t num) & {
	assert(capacity - end_index >= num);

	end_index += num;

	return *this;
}

WeakBuffer&& WeakBuffer::expand_unsafe(size_t num) && {
	return std::move(expand_unsafe(num));
}

void WeakBuffer::read_unsafe(size_t pos, uint8_t* out, size_t size) const {
	assert(this->size() >= size && this->size() - size >= pos);

	std::memcpy(out, data()+pos, size);
}

uint8_t WeakBuffer::read_uint8_unsafe(size_t pos) const {
	uint8_t res;
	read_unsafe(pos, (uint8_t*)&res, 1);

	return res;
}

uint16_t WeakBuffer::read_uint16_unsafe(size_t pos) const {
	uint16_t res;
	read_unsafe(pos, (uint8_t*)&res, 2);

	return res;
}

uint32_t WeakBuffer::read_uint32_unsafe(size_t pos) const {
	uint32_t res;
	read_unsafe(pos, (uint8_t*)&res, 4);

	return res;
}

uint64_t WeakBuffer::read_uint64_unsafe(size_t pos) const {
	uint64_t res;
	read_unsafe(pos, (uint8_t*)&res, 8);

	return res;
}

bool WeakBuffer::read(size_t pos, uint8_t* out, size_t size) const {
	// Bounds checking
	if(this->size() < size || this->size() - size < pos)
		return false;

	read_unsafe(pos, out, size);

	return true;
}

std::optional<uint8_t> WeakBuffer::read_uint8(size_t pos) const {
	// Bounds checking
	if(size() < 1 || size() - 1 < pos)
		return std::nullopt;

	return read_uint8_unsafe(pos);
}

std::optional<uint16_t> WeakBuffer::read_uint16(size_t pos) const {
	// Bounds checking
	if(size() < 2 || size() - 2 < pos)
		return std::nullopt;

	return read_uint16_unsafe(pos);
}

std::optional<uint32_t> WeakBuffer::read_uint32(size_t pos) const {
	// Bounds checking
	if(size() < 4 || size() - 4 < pos)
		return std::nullopt;

	return read_uint32_unsafe(pos);
}

std::optional<uint64_t> WeakBuffer::read_uint64(size_t pos) const {
	// Bounds checking
	if(size() < 8 || size() - 8 < pos)
		return std::nullopt;

	return read_uint64_unsafe(pos);
}

WeakBuffer& WeakBuffer::write_unsafe(size_t pos, uint8_t const* in, size_t size) & {
	assert(this->size() >= size && this->size() - size >= pos);

	std::memcpy(data()+pos, in, size);

	return *this;
}

WeakBuffer&& WeakBuffer::write_unsafe(size_t pos, uint8_t const* in, size_t size) && {
	return std::move(write_unsafe(pos, in, size));
}

WeakBuffer& WeakBuffer::write_uint8_unsafe(size_t pos, uint8_t num) & {
	return write_unsafe(pos, (uint8_t const*)&num, 1);
}

WeakBuffer&& WeakBuffer::write_uint8_unsafe(size_t pos, uint8_t num) && {
	return std::move(write_uint8_unsafe(pos, num));
}

WeakBuffer& WeakBuffer::write_uint16_unsafe(size_t pos, uint16_t num) & {
	return write_unsafe(pos, (uint8_t const*)&num, 2);
}

WeakBuffer&& WeakBuffer::write_uint16_unsafe(size_t pos, uint16_t num) && {
	return std::move(write_uint16_unsafe(pos, num));
}

WeakBuffer& WeakBuffer::write_uint32_unsafe(size_t pos, uint32_t num) & {
	return write_unsafe(pos, (uint8_t const*)&num, 4);
}

WeakBuffer&& WeakBuffer::write_uint32_unsafe(size_t pos, uint32_t num) && {
	return std::move(write_uint32_unsafe(pos, num));
}

WeakBuffer& WeakBuffer::write_uint64_unsafe(size_t pos, uint64_t num) & {
	return write_unsafe(pos, (uint8_t const*)&num, 8);
}

WeakBuffer&& WeakBuffer::write_uint64_unsafe(size_t pos, uint64_t num) && {
	return std::move(write_uint64_unsafe(pos, num));
}

bool WeakBuffer::write(size_t pos, uint8_t const* in, size_t size) {
	// Bounds checking
	if(this->size() < size || this->size() - size < pos)
		return false;

	write_unsafe(pos, in, size);

	return true;
}

bool WeakBuffer::write_uint8(size_t pos, uint8_t num) {
	// Bounds checking
	if(size() < 1 || size() - 1 < pos)
		return false;

	write_uint8_unsafe(pos, num);

	return true;
}

bool WeakBuffer::write_uint16(size_t pos, uint16_t num) {
	// Bounds checking
	if(size() < 2 || size() - 2 < pos)
		return false;

	write_uint16_unsafe(pos, num);

	return true;
}

bool WeakBuffer::write_uint32(size_t pos, uint32_t num) {
	// Bounds checking
	if(size() < 4 || size() - 4 < pos)
		return false;

	write_uint32_unsafe(pos, num);

	return true;
}

bool WeakBuffer::write_uint64(size_t pos, uint64_t num) {
	// Bounds checking
	if(size() < 8 || size() - 8 < pos)
		return false;

	write_uint64_unsafe(pos, num);

	return true;
}

#if MARLIN_CORE_ENDIANNESS == MARLIN_CORE_BIG_ENDIAN

std::optional<uint16_t> WeakBuffer::read_uint16_le(size_t pos) const {
	auto res = read_uint16(pos);
	return res.has_value() ? __builtin_bswap16(res.value()) : res;
}

std::optional<uint32_t> WeakBuffer::read_uint32_le(size_t pos) const {
	auto res = read_uint32(pos);
	return res.has_value() ? __builtin_bswap32(res.value()) : res;
}

std::optional<uint64_t> WeakBuffer::read_uint64_le(size_t pos) const {
	auto res = read_uint64(pos);
	return res.has_value() ? __builtin_bswap64(res.value()) : res;
}

uint16_t WeakBuffer::read_uint16_le_unsafe(size_t pos) const {
	return __builtin_bswap16(read_uint16_unsafe(pos));
}

uint32_t WeakBuffer::read_uint32_le_unsafe(size_t pos) const {
	return __builtin_bswap32(read_uint32_unsafe(pos));
}

uint64_t WeakBuffer::read_uint64_le_unsafe(size_t pos) const {
	return __builtin_bswap64(read_uint64_unsafe(pos));
}

std::optional<uint16_t> WeakBuffer::read_uint16_be(size_t pos) const {
	return read_uint16(pos);
}

std::optional<uint32_t> WeakBuffer::read_uint32_be(size_t pos) const {
	return read_uint32(pos);
}

std::optional<uint64_t> WeakBuffer::read_uint64_be(size_t pos) const {
	return read_uint64(pos);
}

uint16_t WeakBuffer::read_uint16_be_unsafe(size_t pos) const {
	return read_uint16_unsafe(pos);
}

uint32_t WeakBuffer::read_uint32_be_unsafe(size_t pos) const {
	return read_uint32_unsafe(pos);
}

uint64_t WeakBuffer::read_uint64_be_unsafe(size_t pos) const {
	return read_uint64_unsafe(pos);
}

bool WeakBuffer::write_uint16_le(size_t pos, uint16_t num) {
	return write_uint16(pos, __builtin_bswap16(num));
}

bool WeakBuffer::write_uint32_le(size_t pos, uint32_t num) {
	return write_uint32(pos, __builtin_bswap32(num));
}

bool WeakBuffer::write_uint64_le(size_t pos, uint64_t num) {
	return write_uint64(pos, __builtin_bswap64(num));
}

WeakBuffer& WeakBuffer::write_uint16_le_unsafe(size_t pos, uint16_t num) & {
	return write_uint16_unsafe(pos, __builtin_bswap16(num));
}

WeakBuffer&& WeakBuffer::write_uint16_le_unsafe(size_t pos, uint16_t num) && {
	return std::move(write_uint16_le_unsafe(pos, num));
}

WeakBuffer& WeakBuffer::write_uint32_le_unsafe(size_t pos, uint32_t num) & {
	return write_uint32_unsafe(pos, __builtin_bswap32(num));
}

WeakBuffer&& WeakBuffer::write_uint32_le_unsafe(size_t pos, uint32_t num) && {
	return std::move(write_uint32_le_unsafe(pos, num));
}

WeakBuffer& WeakBuffer::write_uint64_le_unsafe(size_t pos, uint64_t num) & {
	return write_uint64_unsafe(pos, __builtin_bswap64(num));
}

WeakBuffer&& WeakBuffer::write_uint64_le_unsafe(size_t pos, uint64_t num) && {
	return std::move(write_uint64_le_unsafe(pos, num));
}

bool WeakBuffer::write_uint16_be(size_t pos, uint16_t num) {
	return write_uint16(pos, num);
}

bool WeakBuffer::write_uint32_be(size_t pos, uint32_t num) {
	return write_uint32(pos, num);
}

bool WeakBuffer::write_uint64_be(size_t pos, uint64_t num) {
	return write_uint64(pos, num);
}

WeakBuffer& WeakBuffer::write_uint16_be_unsafe(size_t pos, uint16_t num) & {
	return write_uint16_unsafe(pos, num);
}

WeakBuffer&& WeakBuffer::write_uint16_be_unsafe(size_t pos, uint16_t num) && {
	return std::move(write_uint16_be_unsafe(pos, num));
}

WeakBuffer& WeakBuffer::write_uint32_be_unsafe(size_t pos, uint32_t num) & {
	return write_uint32_unsafe(pos, num);
}

WeakBuffer&& WeakBuffer::write_uint32_be_unsafe(size_t pos, uint32_t num) && {
	return std::move(write_uint32_be_unsafe(pos, num));
}

WeakBuffer& WeakBuffer::write_uint64_be_unsafe(size_t pos, uint64_t num) & {
	return write_uint64_unsafe(pos, num);
}

WeakBuffer&& WeakBuffer::write_uint64_be_unsafe(size_t pos, uint64_t num) && {
	return std::move(write_uint64_be_unsafe(pos, num));
}

#elif MARLIN_CORE_ENDIANNESS == MARLIN_CORE_LITTLE_ENDIAN

std::optional<uint16_t> WeakBuffer::read_uint16_be(size_t pos) const {
	auto res = read_uint16(pos);
	return res.has_value() ? __builtin_bswap16(res.value()) : res;
}

std::optional<uint32_t> WeakBuffer::read_uint32_be(size_t pos) const {
	auto res = read_uint32(pos);
	return res.has_value() ? __builtin_bswap32(res.value()) : res;
}

std::optional<uint64_t> WeakBuffer::read_uint64_be(size_t pos) const {
	auto res = read_uint64(pos);
	return res.has_value() ? __builtin_bswap64(res.value()) : res;
}

uint16_t WeakBuffer::read_uint16_be_unsafe(size_t pos) const {
	return __builtin_bswap16(read_uint16_unsafe(pos));
}

uint32_t WeakBuffer::read_uint32_be_unsafe(size_t pos) const {
	return __builtin_bswap32(read_uint32_unsafe(pos));
}

uint64_t WeakBuffer::read_uint64_be_unsafe(size_t pos) const {
	return __builtin_bswap64(read_uint64_unsafe(pos));
}

std::optional<uint16_t> WeakBuffer::read_uint16_le(size_t pos) const {
	return read_uint16(pos);
}

std::optional<uint32_t> WeakBuffer::read_uint32_le(size_t pos) const {
	return read_uint32(pos);
}

std::optional<uint64_t> WeakBuffer::read_uint64_le(size_t pos) const {
	return read_uint64(pos);
}

uint16_t WeakBuffer::read_uint16_le_unsafe(size_t pos) const {
	return read_uint16_unsafe(pos);
}

uint32_t WeakBuffer::read_uint32_le_unsafe(size_t pos) const {
	return read_uint32_unsafe(pos);
}

uint64_t WeakBuffer::read_uint64_le_unsafe(size_t pos) const {
	return read_uint64_unsafe(pos);
}

bool WeakBuffer::write_uint16_be(size_t pos, uint16_t num) {
	return write_uint16(pos, __builtin_bswap16(num));
}

bool WeakBuffer::write_uint32_be(size_t pos, uint32_t num) {
	return write_uint32(pos, __builtin_bswap32(num));
}

bool WeakBuffer::write_uint64_be(size_t pos, uint64_t num) {
	return write_uint64(pos, __builtin_bswap64(num));
}

WeakBuffer& WeakBuffer::write_uint16_be_unsafe(size_t pos, uint16_t num) & {
	return write_uint16_unsafe(pos, __builtin_bswap16(num));
}

WeakBuffer&& WeakBuffer::write_uint16_be_unsafe(size_t pos, uint16_t num) && {
	return std::move(write_uint16_be_unsafe(pos, num));
}

WeakBuffer& WeakBuffer::write_uint32_be_unsafe(size_t pos, uint32_t num) & {
	return write_uint32_unsafe(pos, __builtin_bswap32(num));
}

WeakBuffer&& WeakBuffer::write_uint32_be_unsafe(size_t pos, uint32_t num) && {
	return std::move(write_uint32_be_unsafe(pos, num));
}

WeakBuffer& WeakBuffer::write_uint64_be_unsafe(size_t pos, uint64_t num) & {
	return write_uint64_unsafe(pos, __builtin_bswap64(num));
}

WeakBuffer&& WeakBuffer::write_uint64_be_unsafe(size_t pos, uint64_t num) && {
	return std::move(write_uint64_be_unsafe(pos, num));
}

bool WeakBuffer::write_uint16_le(size_t pos, uint16_t num) {
	return write_uint16(pos, num);
}

bool WeakBuffer::write_uint32_le(size_t pos, uint32_t num) {
	return write_uint32(pos, num);
}

bool WeakBuffer::write_uint64_le(size_t pos, uint64_t num) {
	return write_uint64(pos, num);
}

WeakBuffer& WeakBuffer::write_uint16_le_unsafe(size_t pos, uint16_t num) & {
	return write_uint16_unsafe(pos, num);
}

WeakBuffer&& WeakBuffer::write_uint16_le_unsafe(size_t pos, uint16_t num) && {
	return std::move(write_uint16_le_unsafe(pos, num));
}

WeakBuffer& WeakBuffer::write_uint32_le_unsafe(size_t pos, uint32_t num) & {
	return write_uint32_unsafe(pos, num);
}

WeakBuffer&& WeakBuffer::write_uint32_le_unsafe(size_t pos, uint32_t num) && {
	return std::move(write_uint32_le_unsafe(pos, num));
}

WeakBuffer& WeakBuffer::write_uint64_le_unsafe(size_t pos, uint64_t num) & {
	return write_uint64_unsafe(pos, num);
}

WeakBuffer&& WeakBuffer::write_uint64_le_unsafe(size_t pos, uint64_t num) && {
	return std::move(write_uint64_le_unsafe(pos, num));
}

#endif

} // namespace core
} // namespace marlin
