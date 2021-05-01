#include "marlin/core/Buffer.hpp"
#include "marlin/core/Endian.hpp"
#include <cstring>
#include <cassert>
#include <algorithm>

namespace marlin {
namespace core {

Buffer::Buffer(size_t size) :
BaseBuffer(new uint8_t[size], size) {}

Buffer::Buffer(std::initializer_list<uint8_t> il, size_t size) :
BaseBuffer(new uint8_t[size], size) {
	assert(il.size() <= size);
	std::copy(il.begin(), il.end(), buf);
}

Buffer::Buffer(uint8_t *buf, size_t size) :
BaseBuffer(buf, size) {}

Buffer::Buffer(Buffer &&b) noexcept :
BaseBuffer(static_cast<BaseBuffer&&>(std::move(b))) {
	b.buf = nullptr;
	b.capacity = 0;
	b.start_index = 0;
	b.end_index = 0;
}

Buffer &Buffer::operator=(Buffer &&b) noexcept {
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

WeakBuffer Buffer::payload_buffer() & {
	return *this;
}

WeakBuffer const Buffer::payload_buffer() const& {
	return *this;
}

Buffer Buffer::payload_buffer() && {
	return std::move(*this);
}

uint8_t* Buffer::payload() {
	return data();
}

uint8_t const* Buffer::payload() const {
	return data();
}

} // namespace core
} // namespace marlin
