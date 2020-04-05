/*! \file Buffer.hpp
*/

#ifndef MARLIN_CORE_BUFFER_HPP
#define MARLIN_CORE_BUFFER_HPP

#include <stdint.h>
#include <uv.h>
#include <memory>
#include <utility>
//! DONOT REMOVE. FAILS TO COMPILE ON MAC OTHERWISE
#include <array>

namespace marlin {
namespace core {

//! Marlin byte buffer implementation
class Buffer {
	uint8_t *buf;
	size_t capacity;
	size_t start_index;
	size_t end_index;

public:
	/// Construct with given size - preferred constructor
	Buffer(size_t const size);

	/// Construct with initializer list and given size - preferred constructor
	Buffer(std::initializer_list<uint8_t> il, size_t const size);
	// Initializer lists are preferable to partially initialized uint8_t arrays
	// Buffer(new uint8_t[10] {'0','1'}, 10) causes zeroing of remaining 8 bytes
	// Buffer({'0','1'}, 10) doesn't

	/// Construct from uint8_t array - unsafe if uint8_t * isn't obtained from new
	Buffer(uint8_t *const buf, size_t const size);

	/// Move contructor
	Buffer(Buffer &&b) noexcept;

	/// Delete copy contructor
	Buffer(Buffer const &b) = delete;

	/// Move assign
	Buffer &operator=(Buffer &&b) noexcept;

	/// Delete copy assign
	Buffer &operator=(Buffer const &p) = delete;

	~Buffer();

	inline uint8_t *release() {
		uint8_t *_buf = buf;

		buf = nullptr;
		capacity = 0;
		start_index = 0;
		end_index = 0;

		return _buf;
	}

	/// Start of buffer
	inline uint8_t *data() const {
		return buf + start_index;
	}

	/// Length of buffer
	inline size_t size() const {
		return end_index - start_index;
	}

	/// Moves start of buffer forward and covers given number of bytes
	bool cover(size_t const num);
	/// Moves start of buffer forward and covers given number of bytes without bounds checking
	void cover_unsafe(size_t const num);

	/// Moves start of buffer backward and uncovers given number of bytes
	bool uncover(size_t const num);
	/// Moves start of buffer backward and uncovers given number of bytes without bounds checking
	void uncover_unsafe(size_t const num);

	/// Moves end of buffer backward and covers given number of bytes
	bool truncate(size_t const num);
	/// Moves end of buffer backward and covers given number of bytes without bounds checking
	void truncate_unsafe(size_t const num);

	/// Moves end of buffer forward and uncovers given number of bytes
	bool expand(size_t const num);
	/// Moves end of buffer forward and uncovers given number of bytes without bounds checking
	void expand_unsafe(size_t const num);

	//-------- Arbitrary reads begin --------//

	/// Read arbitrary data starting at given byte
	bool read(size_t const pos, uint8_t* const out, size_t const size) const;
	/// Read arbitrary data starting at given byte without bounds checking
	void read_unsafe(size_t const pos, uint8_t* const out, size_t const size) const;

	//-------- Arbitrary reads end --------//


	//-------- Arbitrary writes begin --------//

	/// Write arbitrary data starting at given byte
	bool write(size_t const pos, uint8_t const* const in, size_t const size);
	/// Write arbitrary data starting at given byte without bounds checking
	void write_unsafe(size_t const pos, uint8_t const* const in, size_t const size);

	//-------- Arbitrary writes end --------//


	//-------- 8 bit reads begin --------//

	/// Read uint8_t starting at given byte
	uint8_t read_uint8(size_t const pos) const;
	/// Read uint8_t starting at given byte without bounds checking
	uint8_t read_uint8_unsafe(size_t const pos) const;

	//-------- 8 bit reads end --------//


	//-------- 8 bit writes begin --------//

	/// Write uint8_t starting at given byte
	bool write_uint8(size_t const pos, uint8_t const num);
	/// Write uint8_t starting at given byte without bounds checking
	void write_uint8_unsafe(size_t const pos, uint8_t const num);

	//-------- 8 bit writes end --------//


	//-------- 16 bit reads begin --------//

	/// Read uint16_t starting at given byte
	uint16_t read_uint16(size_t const pos) const;
	/// Read uint16_t starting at given byte without bounds checking
	uint16_t read_uint16_unsafe(size_t const pos) const;

	/// Read uint16_t starting at given byte,
	/// converting from LE to host endian
	uint16_t read_uint16_le(size_t const pos) const;
	/// Read uint16_t starting at given byte without bounds checking,
	/// converting from LE to host endian
	uint16_t read_uint16_le_unsafe(size_t const pos) const;

	/// Read uint16_t starting at given byte,
	/// converting from BE to host endian
	uint16_t read_uint16_be(size_t const pos) const;
	/// Read uint16_t starting at given byte without bounds checking,
	/// converting from BE to host endian
	uint16_t read_uint16_be_unsafe(size_t const pos) const;

	//-------- 16 bit reads end --------//


	//-------- 16 bit writes begin --------//

	/// Write uint16_t starting at given byte
	bool write_uint16(size_t const pos, uint16_t const num);
	/// Write uint16_t starting at given byte without bounds checking
	void write_uint16_unsafe(size_t const pos, uint16_t const num);

	/// Write uint16_t starting at given byte,
	/// converting from host endian to LE
	bool write_uint16_le(size_t const pos, uint16_t const num);
	/// Write uint16_t starting at given byte without bounds checking,
	/// converting from host endian to LE
	void write_uint16_le_unsafe(size_t const pos, uint16_t const num);

	/// Write uint16_t starting at given byte,
	/// converting from host endian to BE
	bool write_uint16_be(size_t const pos, uint16_t const num);
	/// Write uint16_t starting at given byte without bounds checking,
	/// converting from host endian to BE
	void write_uint16_be_unsafe(size_t const pos, uint16_t const num);

	//-------- 16 bit writes end --------//


	//-------- 32 bit reads begin --------//

	/// Read uint32_t starting at given byte
	uint32_t read_uint32(size_t const pos) const;
	/// Read uint32_t starting at given byte without bounds checking
	uint32_t read_uint32_unsafe(size_t const pos) const;

	/// Read uint32_t starting at given byte,
	/// converting from LE to host endian
	uint32_t read_uint32_le(size_t const pos) const;
	/// Read uint32_t starting at given byte without bounds checking,
	/// converting from LE to host endian
	uint32_t read_uint32_le_unsafe(size_t const pos) const;

	/// Read uint32_t starting at given byte,
	/// converting from BE to host endian
	uint32_t read_uint32_be(size_t const pos) const;
	/// Read uint32_t starting at given byte without bounds checking,
	/// converting from BE to host endian
	uint32_t read_uint32_be_unsafe(size_t const pos) const;

	//-------- 32 bit reads end --------//


	//-------- 32 bit writes begin --------//

	/// Write uint32_t starting at given byte
	bool write_uint32(size_t const pos, uint32_t const num);
	/// Write uint32_t starting at given byte without bounds checking
	void write_uint32_unsafe(size_t const pos, uint32_t const num);

	/// Write uint32_t starting at given byte,
	/// converting from host endian to LE
	bool write_uint32_le(size_t const pos, uint32_t const num);
	/// Write uint32_t starting at given byte without bounds checking,
	/// converting from host endian to LE
	void write_uint32_le_unsafe(size_t const pos, uint32_t const num);

	/// Write uint32_t starting at given byte,
	/// converting from host endian to BE
	bool write_uint32_be(size_t const pos, uint32_t const num);
	/// Write uint32_t starting at given byte without bounds checking,
	/// converting from host endian to BE
	void write_uint32_be_unsafe(size_t const pos, uint32_t const num);

	//-------- 32 bit writes end --------//


	//-------- 64 bit reads begin --------//

	/// Read uint64_t starting at given byte
	uint64_t read_uint64(size_t const pos) const;
	/// Read uint64_t starting at given byte without bounds checking
	uint64_t read_uint64_unsafe(size_t const pos) const;

	/// Read uint64_t starting at given byte,
	/// converting from LE to host endian
	uint64_t read_uint64_le(size_t const pos) const;
	/// Read uint64_t starting at given byte without bounds checking,
	/// converting from LE to host endian
	uint64_t read_uint64_le_unsafe(size_t const pos) const;

	/// Read uint64_t starting at given byte,
	/// converting from BE to host endian
	uint64_t read_uint64_be(size_t const pos) const;
	/// Read uint64_t starting at given byte without bounds checking,
	/// converting from BE to host endian
	uint64_t read_uint64_be_unsafe(size_t const pos) const;

	//-------- 64 bit reads end --------//


	//-------- 64 bit writes begin --------//

	/// Write uint64_t starting at given byte
	bool write_uint64(size_t const pos, uint64_t const num);
	/// Write uint64_t starting at given byte without bounds checking
	void write_uint64_unsafe(size_t const pos, uint64_t const num);

	/// Write uint64_t starting at given byte,
	/// converting from host endian to LE
	bool write_uint64_le(size_t const pos, uint64_t const num);
	/// Write uint64_t starting at given byte without bounds checking,
	/// converting from host endian to LE
	void write_uint64_le_unsafe(size_t const pos, uint64_t const num);

	/// Write uint64_t starting at given byte,
	/// converting from host endian to BE
	bool write_uint64_be(size_t const pos, uint64_t const num);
	/// Write uint64_t starting at given byte without bounds checking,
	/// converting from host endian to BE
	void write_uint64_be_unsafe(size_t const pos, uint64_t const num);

	//-------- 64 bit writes end --------//
};

} // namespace core
} // namespace marlin

#endif // MARLIN_CORE_BUFFER_HPP
