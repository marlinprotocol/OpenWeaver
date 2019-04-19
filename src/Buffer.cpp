#include "Buffer.hpp"

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
	capacity = b.capacity;
	start_index = b.start_index;

	return *this;
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

} // namespace net
} // namespace marlin
