#ifndef MARLIN_NET_BUFFER_HPP
#define MARLIN_NET_BUFFER_HPP

#include <stdint.h>
#include <uv.h>
#include <memory>

namespace marlin {
namespace net {

class Buffer {
	std::unique_ptr<char[]> _data;
	size_t _size;
	size_t _start_index;

public:
	/// Construct from unique_ptr
	Buffer(std::unique_ptr<char[]> &&data, const size_t size);

	/// Construct from char array - unsafe if char * isn't obtained from new
	Buffer(char *data, const size_t size);

	/// Move contructor
	Buffer(Buffer &&p);

	/// Delete copy contructor
	Buffer(const Buffer &p) = delete;

	/// Move assign
	Buffer &operator=(Buffer &&p);

	/// Delete copy assign
	Buffer &operator=(const Buffer &p) = delete;

	/// Better alias for base
	inline char *data() const {
		return _data.get() + _start_index;
	}

	/// Better alias for len
	inline size_t size() const {
		return _size - _start_index;
	}

	/// Moves start of buffer forward and covers given number of bytes
	bool cover(const size_t num);

	/// Moves start of buffer backward and uncovers given number of bytes
	bool uncover(const size_t num);
};

} // namespace net
} // namespace marlin

#endif // MARLIN_NET_BUFFER_HPP
