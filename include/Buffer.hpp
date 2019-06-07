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

	~Buffer();

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

	/// Read uint8_t starting at given byte
	uint8_t read_uint8(size_t const pos) const;

	/// Read uint16_t starting at given byte
	uint16_t read_uint16(size_t const pos) const;
	/// Read uint16_t starting at given byte, converting from LE to host endian
	uint16_t read_uint16_le(size_t const pos) const;
	/// Read uint16_t starting at given byte, converting from BE to host endian
	uint16_t read_uint16_be(size_t const pos) const;

	/// Read uint32_t starting at given byte
	uint32_t read_uint32(size_t const pos) const;
	/// Read uint32_t starting at given byte, converting from LE to host endian
	uint32_t read_uint32_le(size_t const pos) const;
	/// Read uint32_t starting at given byte, converting from BE to host endian
	uint32_t read_uint32_be(size_t const pos) const;

	/// Read uint64_t starting at given byte
	uint64_t read_uint64(size_t const pos) const;
	/// Read uint64_t starting at given byte, converting from LE to host endian
	uint64_t read_uint64_le(size_t const pos) const;
	/// Read uint64_t starting at given byte, converting from BE to host endian
	uint64_t read_uint64_be(size_t const pos) const;

	/// Write uint8_t starting at given byte
	bool write_uint8(size_t const pos, uint8_t const num);

	/// Write uint16_t starting at given byte
	bool write_uint16(size_t const pos, uint16_t const num);
	/// Write uint16_t starting at given byte, converting from host endian to LE
	bool write_uint16_le(size_t const pos, uint16_t const num);
	/// Write uint16_t starting at given byte, converting from host endian to BE
	bool write_uint16_be(size_t const pos, uint16_t const num);

	/// Write uint32_t starting at given byte
	bool write_uint32(size_t const pos, uint32_t const num);
	/// Write uint32_t starting at given byte, converting from host endian to LE
	bool write_uint32_le(size_t const pos, uint32_t const num);
	/// Write uint32_t starting at given byte, converting from host endian to BE
	bool write_uint32_be(size_t const pos, uint32_t const num);

	/// Write uint64_t starting at given byte
	bool write_uint64(size_t const pos, uint64_t const num);
	/// Write uint64_t starting at given byte, converting from host endian to LE
	bool write_uint64_le(size_t const pos, uint64_t const num);
	/// Write uint64_t starting at given byte, converting from host endian to BE
	bool write_uint64_be(size_t const pos, uint64_t const num);
};

} // namespace net
} // namespace marlin

#endif // MARLIN_NET_BUFFER_HPP
