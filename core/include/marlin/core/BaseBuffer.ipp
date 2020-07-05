#include "marlin/core/Endian.hpp"
#include <cstring>
#include <cassert>
#include <algorithm>

namespace marlin {
namespace core {

template<typename DerivedBuffer>
BaseBuffer<DerivedBuffer>::BaseBuffer(uint8_t *buf, size_t size) :
buf(buf), capacity(size), start_index(0), end_index(size) {}

template<typename DerivedBuffer>
bool BaseBuffer<DerivedBuffer>::cover(size_t num) {
	// Bounds checking
	if (num > size())
		return false;

	cover_unsafe(num);

	return true;
}

template<typename DerivedBuffer>
DerivedBuffer& BaseBuffer<DerivedBuffer>::cover_unsafe(size_t num) & {
	assert(num <= size());

	start_index += num;

	return *static_cast<DerivedBuffer*>(this);
}

template<typename DerivedBuffer>
DerivedBuffer&& BaseBuffer<DerivedBuffer>::cover_unsafe(size_t num) && {
	return std::move(cover_unsafe(num));
}

template<typename DerivedBuffer>
bool BaseBuffer<DerivedBuffer>::uncover(size_t num) {
	// Bounds checking
	if (start_index < num)
		return false;

	uncover_unsafe(num);

	return true;
}

template<typename DerivedBuffer>
DerivedBuffer& BaseBuffer<DerivedBuffer>::uncover_unsafe(size_t num) & {
	assert(start_index >= num);

	start_index -= num;

	return *static_cast<DerivedBuffer*>(this);
}

template<typename DerivedBuffer>
DerivedBuffer&& BaseBuffer<DerivedBuffer>::uncover_unsafe(size_t num) && {
	return std::move(uncover_unsafe(num));
}

template<typename DerivedBuffer>
bool BaseBuffer<DerivedBuffer>::truncate(size_t num) {
	// Bounds checking
	if (num > size())
		return false;

	truncate_unsafe(num);

	return true;
}

template<typename DerivedBuffer>
DerivedBuffer& BaseBuffer<DerivedBuffer>::truncate_unsafe(size_t num) & {
	assert(num <= size());

	end_index -= num;

	return *static_cast<DerivedBuffer*>(this);
}

template<typename DerivedBuffer>
DerivedBuffer&& BaseBuffer<DerivedBuffer>::truncate_unsafe(size_t num) && {
	return std::move(truncate_unsafe(num));
}

template<typename DerivedBuffer>
bool BaseBuffer<DerivedBuffer>::expand(size_t num) {
	// Bounds checking
	if (capacity - end_index < num)
		return false;

	expand_unsafe(num);

	return true;
}

template<typename DerivedBuffer>
DerivedBuffer& BaseBuffer<DerivedBuffer>::expand_unsafe(size_t num) & {
	assert(capacity - end_index >= num);

	end_index += num;

	return *static_cast<DerivedBuffer*>(this);
}

template<typename DerivedBuffer>
DerivedBuffer&& BaseBuffer<DerivedBuffer>::expand_unsafe(size_t num) && {
	return std::move(expand_unsafe(num));
}

template<typename DerivedBuffer>
void BaseBuffer<DerivedBuffer>::read_unsafe(size_t pos, uint8_t* out, size_t size) const {
	assert(this->size() >= size && this->size() - size >= pos);

	std::memcpy(out, data()+pos, size);
}

template<typename DerivedBuffer>
uint8_t BaseBuffer<DerivedBuffer>::read_uint8_unsafe(size_t pos) const {
	uint8_t res;
	read_unsafe(pos, (uint8_t*)&res, 1);

	return res;
}

template<typename DerivedBuffer>
uint16_t BaseBuffer<DerivedBuffer>::read_uint16_unsafe(size_t pos) const {
	uint16_t res;
	read_unsafe(pos, (uint8_t*)&res, 2);

	return res;
}

template<typename DerivedBuffer>
uint32_t BaseBuffer<DerivedBuffer>::read_uint32_unsafe(size_t pos) const {
	uint32_t res;
	read_unsafe(pos, (uint8_t*)&res, 4);

	return res;
}

template<typename DerivedBuffer>
uint64_t BaseBuffer<DerivedBuffer>::read_uint64_unsafe(size_t pos) const {
	uint64_t res;
	read_unsafe(pos, (uint8_t*)&res, 8);

	return res;
}

template<typename DerivedBuffer>
bool BaseBuffer<DerivedBuffer>::read(size_t pos, uint8_t* out, size_t size) const {
	// Bounds checking
	if(this->size() < size || this->size() - size < pos)
		return false;

	read_unsafe(pos, out, size);

	return true;
}

template<typename DerivedBuffer>
std::optional<uint8_t> BaseBuffer<DerivedBuffer>::read_uint8(size_t pos) const {
	// Bounds checking
	if(size() < 1 || size() - 1 < pos)
		return std::nullopt;

	return read_uint8_unsafe(pos);
}

template<typename DerivedBuffer>
std::optional<uint16_t> BaseBuffer<DerivedBuffer>::read_uint16(size_t pos) const {
	// Bounds checking
	if(size() < 2 || size() - 2 < pos)
		return std::nullopt;

	return read_uint16_unsafe(pos);
}

template<typename DerivedBuffer>
std::optional<uint32_t> BaseBuffer<DerivedBuffer>::read_uint32(size_t pos) const {
	// Bounds checking
	if(size() < 4 || size() - 4 < pos)
		return std::nullopt;

	return read_uint32_unsafe(pos);
}

template<typename DerivedBuffer>
std::optional<uint64_t> BaseBuffer<DerivedBuffer>::read_uint64(size_t pos) const {
	// Bounds checking
	if(size() < 8 || size() - 8 < pos)
		return std::nullopt;

	return read_uint64_unsafe(pos);
}

template<typename DerivedBuffer>
DerivedBuffer& BaseBuffer<DerivedBuffer>::write_unsafe(size_t pos, uint8_t const* in, size_t size) & {
	assert(this->size() >= size && this->size() - size >= pos);

	std::memcpy(data()+pos, in, size);

	return *static_cast<DerivedBuffer*>(this);
}

template<typename DerivedBuffer>
DerivedBuffer&& BaseBuffer<DerivedBuffer>::write_unsafe(size_t pos, uint8_t const* in, size_t size) && {
	return std::move(write_unsafe(pos, in, size));
}

template<typename DerivedBuffer>
DerivedBuffer& BaseBuffer<DerivedBuffer>::write_uint8_unsafe(size_t pos, uint8_t num) & {
	return write_unsafe(pos, (uint8_t const*)&num, 1);
}

template<typename DerivedBuffer>
DerivedBuffer&& BaseBuffer<DerivedBuffer>::write_uint8_unsafe(size_t pos, uint8_t num) && {
	return std::move(write_uint8_unsafe(pos, num));
}

template<typename DerivedBuffer>
DerivedBuffer& BaseBuffer<DerivedBuffer>::write_uint16_unsafe(size_t pos, uint16_t num) & {
	return write_unsafe(pos, (uint8_t const*)&num, 2);
}

template<typename DerivedBuffer>
DerivedBuffer&& BaseBuffer<DerivedBuffer>::write_uint16_unsafe(size_t pos, uint16_t num) && {
	return std::move(write_uint16_unsafe(pos, num));
}

template<typename DerivedBuffer>
DerivedBuffer& BaseBuffer<DerivedBuffer>::write_uint32_unsafe(size_t pos, uint32_t num) & {
	return write_unsafe(pos, (uint8_t const*)&num, 4);
}

template<typename DerivedBuffer>
DerivedBuffer&& BaseBuffer<DerivedBuffer>::write_uint32_unsafe(size_t pos, uint32_t num) && {
	return std::move(write_uint32_unsafe(pos, num));
}

template<typename DerivedBuffer>
DerivedBuffer& BaseBuffer<DerivedBuffer>::write_uint64_unsafe(size_t pos, uint64_t num) & {
	return write_unsafe(pos, (uint8_t const*)&num, 8);
}

template<typename DerivedBuffer>
DerivedBuffer&& BaseBuffer<DerivedBuffer>::write_uint64_unsafe(size_t pos, uint64_t num) && {
	return std::move(write_uint64_unsafe(pos, num));
}

template<typename DerivedBuffer>
bool BaseBuffer<DerivedBuffer>::write(size_t pos, uint8_t const* in, size_t size) {
	// Bounds checking
	if(this->size() < size || this->size() - size < pos)
		return false;

	write_unsafe(pos, in, size);

	return true;
}

template<typename DerivedBuffer>
bool BaseBuffer<DerivedBuffer>::write_uint8(size_t pos, uint8_t num) {
	// Bounds checking
	if(size() < 1 || size() - 1 < pos)
		return false;

	write_uint8_unsafe(pos, num);

	return true;
}

template<typename DerivedBuffer>
bool BaseBuffer<DerivedBuffer>::write_uint16(size_t pos, uint16_t num) {
	// Bounds checking
	if(size() < 2 || size() - 2 < pos)
		return false;

	write_uint16_unsafe(pos, num);

	return true;
}

template<typename DerivedBuffer>
bool BaseBuffer<DerivedBuffer>::write_uint32(size_t pos, uint32_t num) {
	// Bounds checking
	if(size() < 4 || size() - 4 < pos)
		return false;

	write_uint32_unsafe(pos, num);

	return true;
}

template<typename DerivedBuffer>
bool BaseBuffer<DerivedBuffer>::write_uint64(size_t pos, uint64_t num) {
	// Bounds checking
	if(size() < 8 || size() - 8 < pos)
		return false;

	write_uint64_unsafe(pos, num);

	return true;
}

#if MARLIN_CORE_ENDIANNESS == MARLIN_CORE_BIG_ENDIAN

template<typename DerivedBuffer>
std::optional<uint16_t> BaseBuffer<DerivedBuffer>::read_uint16_le(size_t pos) const {
	auto res = read_uint16(pos);
	return res.has_value() ? __builtin_bswap16(res.value()) : res;
}

template<typename DerivedBuffer>
std::optional<uint32_t> BaseBuffer<DerivedBuffer>::read_uint32_le(size_t pos) const {
	auto res = read_uint32(pos);
	return res.has_value() ? __builtin_bswap32(res.value()) : res;
}

template<typename DerivedBuffer>
std::optional<uint64_t> BaseBuffer<DerivedBuffer>::read_uint64_le(size_t pos) const {
	auto res = read_uint64(pos);
	return res.has_value() ? __builtin_bswap64(res.value()) : res;
}

template<typename DerivedBuffer>
uint16_t BaseBuffer<DerivedBuffer>::read_uint16_le_unsafe(size_t pos) const {
	return __builtin_bswap16(read_uint16_unsafe(pos));
}

template<typename DerivedBuffer>
uint32_t BaseBuffer<DerivedBuffer>::read_uint32_le_unsafe(size_t pos) const {
	return __builtin_bswap32(read_uint32_unsafe(pos));
}

template<typename DerivedBuffer>
uint64_t BaseBuffer<DerivedBuffer>::read_uint64_le_unsafe(size_t pos) const {
	return __builtin_bswap64(read_uint64_unsafe(pos));
}

template<typename DerivedBuffer>
std::optional<uint16_t> BaseBuffer<DerivedBuffer>::read_uint16_be(size_t pos) const {
	return read_uint16(pos);
}

template<typename DerivedBuffer>
std::optional<uint32_t> BaseBuffer<DerivedBuffer>::read_uint32_be(size_t pos) const {
	return read_uint32(pos);
}

template<typename DerivedBuffer>
std::optional<uint64_t> BaseBuffer<DerivedBuffer>::read_uint64_be(size_t pos) const {
	return read_uint64(pos);
}

template<typename DerivedBuffer>
uint16_t BaseBuffer<DerivedBuffer>::read_uint16_be_unsafe(size_t pos) const {
	return read_uint16_unsafe(pos);
}

template<typename DerivedBuffer>
uint32_t BaseBuffer<DerivedBuffer>::read_uint32_be_unsafe(size_t pos) const {
	return read_uint32_unsafe(pos);
}

template<typename DerivedBuffer>
uint64_t BaseBuffer<DerivedBuffer>::read_uint64_be_unsafe(size_t pos) const {
	return read_uint64_unsafe(pos);
}

template<typename DerivedBuffer>
bool BaseBuffer<DerivedBuffer>::write_uint16_le(size_t pos, uint16_t num) {
	return write_uint16(pos, __builtin_bswap16(num));
}

template<typename DerivedBuffer>
bool BaseBuffer<DerivedBuffer>::write_uint32_le(size_t pos, uint32_t num) {
	return write_uint32(pos, __builtin_bswap32(num));
}

template<typename DerivedBuffer>
bool BaseBuffer<DerivedBuffer>::write_uint64_le(size_t pos, uint64_t num) {
	return write_uint64(pos, __builtin_bswap64(num));
}

template<typename DerivedBuffer>
DerivedBuffer& BaseBuffer<DerivedBuffer>::write_uint16_le_unsafe(size_t pos, uint16_t num) & {
	return write_uint16_unsafe(pos, __builtin_bswap16(num));
}

template<typename DerivedBuffer>
DerivedBuffer&& BaseBuffer<DerivedBuffer>::write_uint16_le_unsafe(size_t pos, uint16_t num) && {
	return std::move(write_uint16_le_unsafe(pos, num));
}

template<typename DerivedBuffer>
DerivedBuffer& BaseBuffer<DerivedBuffer>::write_uint32_le_unsafe(size_t pos, uint32_t num) & {
	return write_uint32_unsafe(pos, __builtin_bswap32(num));
}

template<typename DerivedBuffer>
DerivedBuffer&& BaseBuffer<DerivedBuffer>::write_uint32_le_unsafe(size_t pos, uint32_t num) && {
	return std::move(write_uint32_le_unsafe(pos, num));
}

template<typename DerivedBuffer>
DerivedBuffer& BaseBuffer<DerivedBuffer>::write_uint64_le_unsafe(size_t pos, uint64_t num) & {
	return write_uint64_unsafe(pos, __builtin_bswap64(num));
}

template<typename DerivedBuffer>
DerivedBuffer&& BaseBuffer<DerivedBuffer>::write_uint64_le_unsafe(size_t pos, uint64_t num) && {
	return std::move(write_uint64_le_unsafe(pos, num));
}

template<typename DerivedBuffer>
bool BaseBuffer<DerivedBuffer>::write_uint16_be(size_t pos, uint16_t num) {
	return write_uint16(pos, num);
}

template<typename DerivedBuffer>
bool BaseBuffer<DerivedBuffer>::write_uint32_be(size_t pos, uint32_t num) {
	return write_uint32(pos, num);
}

template<typename DerivedBuffer>
bool BaseBuffer<DerivedBuffer>::write_uint64_be(size_t pos, uint64_t num) {
	return write_uint64(pos, num);
}

template<typename DerivedBuffer>
DerivedBuffer& BaseBuffer<DerivedBuffer>::write_uint16_be_unsafe(size_t pos, uint16_t num) & {
	return write_uint16_unsafe(pos, num);
}

template<typename DerivedBuffer>
DerivedBuffer&& BaseBuffer<DerivedBuffer>::write_uint16_be_unsafe(size_t pos, uint16_t num) && {
	return std::move(write_uint16_be_unsafe(pos, num));
}

template<typename DerivedBuffer>
DerivedBuffer& BaseBuffer<DerivedBuffer>::write_uint32_be_unsafe(size_t pos, uint32_t num) & {
	return write_uint32_unsafe(pos, num);
}

template<typename DerivedBuffer>
DerivedBuffer&& BaseBuffer<DerivedBuffer>::write_uint32_be_unsafe(size_t pos, uint32_t num) && {
	return std::move(write_uint32_be_unsafe(pos, num));
}

template<typename DerivedBuffer>
DerivedBuffer& BaseBuffer<DerivedBuffer>::write_uint64_be_unsafe(size_t pos, uint64_t num) & {
	return write_uint64_unsafe(pos, num);
}

template<typename DerivedBuffer>
DerivedBuffer&& BaseBuffer<DerivedBuffer>::write_uint64_be_unsafe(size_t pos, uint64_t num) && {
	return std::move(write_uint64_be_unsafe(pos, num));
}

#elif MARLIN_CORE_ENDIANNESS == MARLIN_CORE_LITTLE_ENDIAN

template<typename DerivedBuffer>
std::optional<uint16_t> BaseBuffer<DerivedBuffer>::read_uint16_be(size_t pos) const {
	auto res = read_uint16(pos);
	return res.has_value() ? __builtin_bswap16(res.value()) : res;
}

template<typename DerivedBuffer>
std::optional<uint32_t> BaseBuffer<DerivedBuffer>::read_uint32_be(size_t pos) const {
	auto res = read_uint32(pos);
	return res.has_value() ? __builtin_bswap32(res.value()) : res;
}

template<typename DerivedBuffer>
std::optional<uint64_t> BaseBuffer<DerivedBuffer>::read_uint64_be(size_t pos) const {
	auto res = read_uint64(pos);
	return res.has_value() ? __builtin_bswap64(res.value()) : res;
}

template<typename DerivedBuffer>
uint16_t BaseBuffer<DerivedBuffer>::read_uint16_be_unsafe(size_t pos) const {
	return __builtin_bswap16(read_uint16_unsafe(pos));
}

template<typename DerivedBuffer>
uint32_t BaseBuffer<DerivedBuffer>::read_uint32_be_unsafe(size_t pos) const {
	return __builtin_bswap32(read_uint32_unsafe(pos));
}

template<typename DerivedBuffer>
uint64_t BaseBuffer<DerivedBuffer>::read_uint64_be_unsafe(size_t pos) const {
	return __builtin_bswap64(read_uint64_unsafe(pos));
}

template<typename DerivedBuffer>
std::optional<uint16_t> BaseBuffer<DerivedBuffer>::read_uint16_le(size_t pos) const {
	return read_uint16(pos);
}

template<typename DerivedBuffer>
std::optional<uint32_t> BaseBuffer<DerivedBuffer>::read_uint32_le(size_t pos) const {
	return read_uint32(pos);
}

template<typename DerivedBuffer>
std::optional<uint64_t> BaseBuffer<DerivedBuffer>::read_uint64_le(size_t pos) const {
	return read_uint64(pos);
}

template<typename DerivedBuffer>
uint16_t BaseBuffer<DerivedBuffer>::read_uint16_le_unsafe(size_t pos) const {
	return read_uint16_unsafe(pos);
}

template<typename DerivedBuffer>
uint32_t BaseBuffer<DerivedBuffer>::read_uint32_le_unsafe(size_t pos) const {
	return read_uint32_unsafe(pos);
}

template<typename DerivedBuffer>
uint64_t BaseBuffer<DerivedBuffer>::read_uint64_le_unsafe(size_t pos) const {
	return read_uint64_unsafe(pos);
}

template<typename DerivedBuffer>
bool BaseBuffer<DerivedBuffer>::write_uint16_be(size_t pos, uint16_t num) {
	return write_uint16(pos, __builtin_bswap16(num));
}

template<typename DerivedBuffer>
bool BaseBuffer<DerivedBuffer>::write_uint32_be(size_t pos, uint32_t num) {
	return write_uint32(pos, __builtin_bswap32(num));
}

template<typename DerivedBuffer>
bool BaseBuffer<DerivedBuffer>::write_uint64_be(size_t pos, uint64_t num) {
	return write_uint64(pos, __builtin_bswap64(num));
}

template<typename DerivedBuffer>
DerivedBuffer& BaseBuffer<DerivedBuffer>::write_uint16_be_unsafe(size_t pos, uint16_t num) & {
	return write_uint16_unsafe(pos, __builtin_bswap16(num));
}

template<typename DerivedBuffer>
DerivedBuffer&& BaseBuffer<DerivedBuffer>::write_uint16_be_unsafe(size_t pos, uint16_t num) && {
	return std::move(write_uint16_be_unsafe(pos, num));
}

template<typename DerivedBuffer>
DerivedBuffer& BaseBuffer<DerivedBuffer>::write_uint32_be_unsafe(size_t pos, uint32_t num) & {
	return write_uint32_unsafe(pos, __builtin_bswap32(num));
}

template<typename DerivedBuffer>
DerivedBuffer&& BaseBuffer<DerivedBuffer>::write_uint32_be_unsafe(size_t pos, uint32_t num) && {
	return std::move(write_uint32_be_unsafe(pos, num));
}

template<typename DerivedBuffer>
DerivedBuffer& BaseBuffer<DerivedBuffer>::write_uint64_be_unsafe(size_t pos, uint64_t num) & {
	return write_uint64_unsafe(pos, __builtin_bswap64(num));
}

template<typename DerivedBuffer>
DerivedBuffer&& BaseBuffer<DerivedBuffer>::write_uint64_be_unsafe(size_t pos, uint64_t num) && {
	return std::move(write_uint64_be_unsafe(pos, num));
}

template<typename DerivedBuffer>
bool BaseBuffer<DerivedBuffer>::write_uint16_le(size_t pos, uint16_t num) {
	return write_uint16(pos, num);
}

template<typename DerivedBuffer>
bool BaseBuffer<DerivedBuffer>::write_uint32_le(size_t pos, uint32_t num) {
	return write_uint32(pos, num);
}

template<typename DerivedBuffer>
bool BaseBuffer<DerivedBuffer>::write_uint64_le(size_t pos, uint64_t num) {
	return write_uint64(pos, num);
}

template<typename DerivedBuffer>
DerivedBuffer& BaseBuffer<DerivedBuffer>::write_uint16_le_unsafe(size_t pos, uint16_t num) & {
	return write_uint16_unsafe(pos, num);
}

template<typename DerivedBuffer>
DerivedBuffer&& BaseBuffer<DerivedBuffer>::write_uint16_le_unsafe(size_t pos, uint16_t num) && {
	return std::move(write_uint16_le_unsafe(pos, num));
}

template<typename DerivedBuffer>
DerivedBuffer& BaseBuffer<DerivedBuffer>::write_uint32_le_unsafe(size_t pos, uint32_t num) & {
	return write_uint32_unsafe(pos, num);
}

template<typename DerivedBuffer>
DerivedBuffer&& BaseBuffer<DerivedBuffer>::write_uint32_le_unsafe(size_t pos, uint32_t num) && {
	return std::move(write_uint32_le_unsafe(pos, num));
}

template<typename DerivedBuffer>
DerivedBuffer& BaseBuffer<DerivedBuffer>::write_uint64_le_unsafe(size_t pos, uint64_t num) & {
	return write_uint64_unsafe(pos, num);
}

template<typename DerivedBuffer>
DerivedBuffer&& BaseBuffer<DerivedBuffer>::write_uint64_le_unsafe(size_t pos, uint64_t num) && {
	return std::move(write_uint64_le_unsafe(pos, num));
}

#endif

} // namespace core
} // namespace marlin
