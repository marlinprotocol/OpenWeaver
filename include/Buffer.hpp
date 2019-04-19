#ifndef MARLIN_NET_BUFFER_HPP
#define MARLIN_NET_BUFFER_HPP

#include <stdint.h>
#include <uv.h>
#include <memory>

namespace marlin {
namespace net {

class Buffer {
	char *buf;
	size_t capacity;
	size_t start_index;

public:
	/// Construct from unique_ptr
	Buffer(std::unique_ptr<char[]> &&buf, size_t const size);

	/// Construct from char array - unsafe if char * isn't obtained from new
	Buffer(char *const buf, size_t const size);

	/// Move contructor
	Buffer(Buffer &&b);

	/// Delete copy contructor
	Buffer(Buffer const &b) = delete;

	/// Move assign
	Buffer &operator=(Buffer &&b);

	/// Delete copy assign
	Buffer &operator=(Buffer const &p) = delete;

	/// Start of buffer
	inline char *data() const {
		return buf + start_index;
	}

	/// Length of buffer
	inline size_t size() const {
		return capacity - start_index;
	}

	/// Moves start of buffer forward and covers given number of bytes
	bool cover(size_t const num);

	/// Moves start of buffer backward and uncovers given number of bytes
	bool uncover(size_t const num);
};

} // namespace net
} // namespace marlin

#endif // MARLIN_NET_BUFFER_HPP
