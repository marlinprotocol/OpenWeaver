#include "Buffer.hpp"

namespace marlin {
namespace net {

Buffer::Buffer(std::unique_ptr<char[]> &&data, const size_t size) : _data(std::move(data)) {
	this->_size = size;
	this->_start_index = 0;
}

Buffer::Buffer(char *data, const size_t size) : _data(data) {
	this->_size = size;
	this->_start_index = 0;
}

Buffer::Buffer(Buffer &&p) {
	_data = std::move(p._data);
	_size = p._size;
	_start_index = p._start_index;
}

Buffer &Buffer::operator=(Buffer &&p) {
	_data = std::move(p._data);
	_size = p._size;
	_start_index = p._start_index;

	return *this;
}

bool Buffer::cover(const size_t num) {
	// Bounds checking
	if (_start_index + num >= _size)
		return false;

	_start_index += num;
	return true;
}

bool Buffer::uncover(const size_t num) {
	// Bounds checking
	if (_start_index < num)
		return false;

	_start_index -= num;
	return true;
}

} // namespace net
} // namespace marlin
