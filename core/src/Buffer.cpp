#include "marlin/core/Buffer.hpp"
#include "marlin/core/Endian.hpp"
#include <cstring>
#include <cassert>
#include <algorithm>

namespace marlin {
namespace core {

Buffer::Buffer(size_t size) :
WeakBuffer(new uint8_t[size], size) {}

Buffer::Buffer(std::initializer_list<uint8_t> il, size_t size) :
WeakBuffer(new uint8_t[size], size) {
	assert(il.size() <= size);
	std::copy(il.begin(), il.end(), buf);
}

Buffer::Buffer(uint8_t *buf, size_t size) :
WeakBuffer(buf, size) {}

Buffer::Buffer(Buffer &&b) noexcept :
WeakBuffer(static_cast<WeakBuffer&&>(std::move(b))) {
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

} // namespace core
} // namespace marlin
