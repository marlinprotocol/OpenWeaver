#ifndef MARLIN_NET_PACKET_HPP
#define MARLIN_NET_PACKET_HPP

#include <stdint.h>
#include <uv.h>
#include <memory>

namespace marlin {
namespace net {

class Packet {
	std::unique_ptr<char[]> _data;
	size_t _size;
	size_t _start_index;

public:
	/// Construct from unique_ptr
	Packet(std::unique_ptr<char[]> &&data, const size_t size);

	/// Construct from char array - unsafe if char * isn't obtained from new
	Packet(char *data, const size_t size);

	/// Move contructor
	Packet(Packet &&p);

	/// Delete copy contructor
	Packet(const Packet &p) = delete;

	/// Move assign
	Packet &operator=(Packet &&p);

	/// Delete copy assign
	Packet &operator=(const Packet &p) = delete;

	/// Better alias for base
	inline char *data() const {
		return _data.get() + _start_index;
	}

	/// Better alias for len
	inline size_t size() const {
		return _size - _start_index;
	}

	/// Moves start of packet forward and covers given number of bytes
	bool cover(const size_t num);

	/// Moves start of packet backward and uncovers given number of bytes
	bool uncover(const size_t num);
};

} // namespace net
} // namespace marlin

#endif // MARLIN_NET_PACKET_HPP
