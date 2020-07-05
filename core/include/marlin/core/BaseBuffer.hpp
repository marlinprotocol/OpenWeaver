#ifndef MARLIN_CORE_BASEBUFFER_HPP
#define MARLIN_CORE_BASEBUFFER_HPP

#include <stdint.h>
#include <uv.h>
#include <memory>
#include <utility>
#include <optional>
// DONOT REMOVE. FAILS TO COMPILE ON MAC OTHERWISE
#include <array>

namespace marlin {
namespace core {

/// @brief Byte buffer implementation with modifiable bounds
/// @headerfile BaseBuffer.hpp <marlin/core/BaseBuffer.hpp>
template<typename DerivedBuffer>
class BaseBuffer {
protected:
	/// Pointer to underlying memory
	uint8_t *buf;
	/// Capacity of memory
	size_t capacity;
	/// Start index in memory, inclusive
	size_t start_index;
	/// End index in memory, non-inclusive
	size_t end_index;

public:
	/// @brief Construct from uint8_t array
	/// @param buf Pointer to bytes
	/// @param size Size of bytes
	BaseBuffer(uint8_t *buf, size_t size);

	/// Move contructor
	BaseBuffer(BaseBuffer &&b) = default;

	/// Copy contructor
	BaseBuffer(BaseBuffer const &b) = default;

	/// Move assign
	BaseBuffer &operator=(BaseBuffer &&b) = default;

	/// Copy assign
	BaseBuffer &operator=(BaseBuffer const &b) = default;

	/// Start of buffer
	inline uint8_t *data() {
		return buf + start_index;
	}

	inline uint8_t const *data() const {
		return buf + start_index;
	}

	/// Length of buffer
	inline size_t size() const {
		return end_index - start_index;
	}

	/// @name Bounds change
	/// @{

	//! Moves start of buffer forward and covers given number of bytes
	[[nodiscard]] bool cover(size_t num);
	/// Moves start of buffer forward and covers given number of bytes without bounds checking
	DerivedBuffer& cover_unsafe(size_t num) &;
	DerivedBuffer&& cover_unsafe(size_t num) &&;

	/// Moves start of buffer backward and uncovers given number of bytes
	[[nodiscard]] bool uncover(size_t num);
	/// Moves start of buffer backward and uncovers given number of bytes without bounds checking
	DerivedBuffer& uncover_unsafe(size_t num) &;
	DerivedBuffer&& uncover_unsafe(size_t num) &&;

	/// Moves end of buffer backward and covers given number of bytes
	[[nodiscard]] bool truncate(size_t num);
	/// Moves end of buffer backward and covers given number of bytes without bounds checking
	DerivedBuffer& truncate_unsafe(size_t num) &;
	DerivedBuffer&& truncate_unsafe(size_t num) &&;

	/// Moves end of buffer forward and uncovers given number of bytes
	[[nodiscard]] bool expand(size_t num);
	/// Moves end of buffer forward and uncovers given number of bytes without bounds checking
	DerivedBuffer& expand_unsafe(size_t num) &;
	DerivedBuffer&& expand_unsafe(size_t num) &&;
	/// @}

	/// @name Read/Write arbitrary data
	/// @{
	//-------- Arbitrary reads begin --------//

	/// Read arbitrary data starting at given byte
	[[nodiscard]] bool read(size_t pos, uint8_t* out, size_t size) const;
	/// Read arbitrary data starting at given byte without bounds checking
	void read_unsafe(size_t pos, uint8_t* out, size_t size) const;

	//-------- Arbitrary reads end --------//

	//-------- Arbitrary writes begin --------//

	/// Write arbitrary data starting at given byte
	[[nodiscard]] bool write(size_t pos, uint8_t const* in, size_t size);
	/// Write arbitrary data starting at given byte without bounds checking
	DerivedBuffer& write_unsafe(size_t pos, uint8_t const* in, size_t size) &;
	DerivedBuffer&& write_unsafe(size_t pos, uint8_t const* in, size_t size) &&;

	//-------- Arbitrary writes end --------//
	/// @}

	/// @name Read/Write uint8_t
	/// @{
	//-------- 8 bit reads begin --------//

	/// Read uint8_t starting at given byte
	std::optional<uint8_t> read_uint8(size_t pos) const;
	/// Read uint8_t starting at given byte without bounds checking
	uint8_t read_uint8_unsafe(size_t pos) const;

	//-------- 8 bit reads end --------//


	//-------- 8 bit writes begin --------//

	/// Write uint8_t starting at given byte
	[[nodiscard]] bool write_uint8(size_t pos, uint8_t num);
	/// Write uint8_t starting at given byte without bounds checking
	DerivedBuffer& write_uint8_unsafe(size_t pos, uint8_t num) &;
	DerivedBuffer&& write_uint8_unsafe(size_t pos, uint8_t num) &&;

	//-------- 8 bit writes end --------//
	/// @}

	/// @name Read/Write uint16_t
	/// @{
	//-------- 16 bit reads begin --------//

	/// Read uint16_t starting at given byte
	std::optional<uint16_t> read_uint16(size_t pos) const;
	/// Read uint16_t starting at given byte without bounds checking
	uint16_t read_uint16_unsafe(size_t pos) const;

	/// Read uint16_t starting at given byte,
	/// converting from LE to host endian
	std::optional<uint16_t> read_uint16_le(size_t pos) const;

	/// Read uint16_t starting at given byte without bounds checking,
	/// converting from LE to host endian
	uint16_t read_uint16_le_unsafe(size_t pos) const;

	/// Read uint16_t starting at given byte,
	/// converting from BE to host endian
	std::optional<uint16_t> read_uint16_be(size_t pos) const;
	/// Read uint16_t starting at given byte without bounds checking,
	/// converting from BE to host endian
	uint16_t read_uint16_be_unsafe(size_t pos) const;

	//-------- 16 bit reads end --------//

	//-------- 16 bit writes begin --------//

	/// Write uint16_t starting at given byte
	[[nodiscard]] bool write_uint16(size_t pos, uint16_t num);
	/// Write uint16_t starting at given byte without bounds checking
	DerivedBuffer& write_uint16_unsafe(size_t pos, uint16_t num) &;
	DerivedBuffer&& write_uint16_unsafe(size_t pos, uint16_t num) &&;

	/// Write uint16_t starting at given byte,
	/// converting from host endian to LE
	[[nodiscard]] bool write_uint16_le(size_t pos, uint16_t num);
	/// Write uint16_t starting at given byte without bounds checking,
	/// converting from host endian to LE
	DerivedBuffer& write_uint16_le_unsafe(size_t pos, uint16_t num) &;
	DerivedBuffer&& write_uint16_le_unsafe(size_t pos, uint16_t num) &&;

	/// Write uint16_t starting at given byte,
	/// converting from host endian to BE
	[[nodiscard]] bool write_uint16_be(size_t pos, uint16_t num);
	/// Write uint16_t starting at given byte without bounds checking,
	/// converting from host endian to BE
	DerivedBuffer& write_uint16_be_unsafe(size_t pos, uint16_t num) &;
	DerivedBuffer&& write_uint16_be_unsafe(size_t pos, uint16_t num) &&;

	//-------- 16 bit writes end --------//
	/// @}

	/// @name Read/Write uint32_t
	/// @{
	//-------- 32 bit reads begin --------//

	/// Read uint32_t starting at given byte
	std::optional<uint32_t> read_uint32(size_t pos) const;
	/// Read uint32_t starting at given byte without bounds checking
	uint32_t read_uint32_unsafe(size_t pos) const;

	/// Read uint32_t starting at given byte,
	/// converting from LE to host endian
	std::optional<uint32_t> read_uint32_le(size_t pos) const;
	/// Read uint32_t starting at given byte without bounds checking,
	/// converting from LE to host endian
	uint32_t read_uint32_le_unsafe(size_t pos) const;

	/// Read uint32_t starting at given byte,
	/// converting from BE to host endian
	std::optional<uint32_t> read_uint32_be(size_t pos) const;
	/// Read uint32_t starting at given byte without bounds checking,
	/// converting from BE to host endian
	uint32_t read_uint32_be_unsafe(size_t pos) const;

	//-------- 32 bit reads end --------//

	//-------- 32 bit writes begin --------//

	/// Write uint32_t starting at given byte
	[[nodiscard]] bool write_uint32(size_t pos, uint32_t num);
	/// Write uint32_t starting at given byte without bounds checking
	DerivedBuffer& write_uint32_unsafe(size_t pos, uint32_t num) &;
	DerivedBuffer&& write_uint32_unsafe(size_t pos, uint32_t num) &&;

	/// Write uint32_t starting at given byte,
	/// converting from host endian to LE
	[[nodiscard]] bool write_uint32_le(size_t pos, uint32_t num);
	/// Write uint32_t starting at given byte without bounds checking,
	/// converting from host endian to LE
	DerivedBuffer& write_uint32_le_unsafe(size_t pos, uint32_t num) &;
	DerivedBuffer&& write_uint32_le_unsafe(size_t pos, uint32_t num) &&;

	/// Write uint32_t starting at given byte,
	/// converting from host endian to BE
	[[nodiscard]] bool write_uint32_be(size_t pos, uint32_t num);
	/// Write uint32_t starting at given byte without bounds checking,
	/// converting from host endian to BE
	DerivedBuffer& write_uint32_be_unsafe(size_t pos, uint32_t num) &;
	DerivedBuffer&& write_uint32_be_unsafe(size_t pos, uint32_t num) &&;

	//-------- 32 bit writes end --------//
	/// @}

	/// @name Read/Write uint64_t
	/// @{
	//-------- 64 bit reads begin --------//

	/// Read uint64_t starting at given byte
	std::optional<uint64_t> read_uint64(size_t pos) const;
	/// Read uint64_t starting at given byte without bounds checking
	uint64_t read_uint64_unsafe(size_t pos) const;

	/// Read uint64_t starting at given byte,
	/// converting from LE to host endian
	std::optional<uint64_t> read_uint64_le(size_t pos) const;
	/// Read uint64_t starting at given byte without bounds checking,
	/// converting from LE to host endian
	uint64_t read_uint64_le_unsafe(size_t pos) const;

	/// Read uint64_t starting at given byte,
	/// converting from BE to host endian
	std::optional<uint64_t> read_uint64_be(size_t pos) const;
	/// Read uint64_t starting at given byte without bounds checking,
	/// converting from BE to host endian
	uint64_t read_uint64_be_unsafe(size_t pos) const;

	//-------- 64 bit reads end --------//

	//-------- 64 bit writes begin --------//

	/// Write uint64_t starting at given byte
	[[nodiscard]] bool write_uint64(size_t pos, uint64_t num);
	/// Write uint64_t starting at given byte without bounds checking
	DerivedBuffer& write_uint64_unsafe(size_t pos, uint64_t num) &;
	DerivedBuffer&& write_uint64_unsafe(size_t pos, uint64_t num) &&;

	/// Write uint64_t starting at given byte,
	/// converting from host endian to LE
	[[nodiscard]] bool write_uint64_le(size_t pos, uint64_t num);
	/// Write uint64_t starting at given byte without bounds checking,
	/// converting from host endian to LE
	DerivedBuffer& write_uint64_le_unsafe(size_t pos, uint64_t num) &;
	DerivedBuffer&& write_uint64_le_unsafe(size_t pos, uint64_t num) &&;

	/// Write uint64_t starting at given byte,
	/// converting from host endian to BE
	[[nodiscard]] bool write_uint64_be(size_t pos, uint64_t num);
	/// Write uint64_t starting at given byte without bounds checking,
	/// converting from host endian to BE
	DerivedBuffer& write_uint64_be_unsafe(size_t pos, uint64_t num) &;
	DerivedBuffer&& write_uint64_be_unsafe(size_t pos, uint64_t num) &&;

	//-------- 64 bit writes end --------//
	/// @}
};

} // namespace core
} // namespace marlin

#include "marlin/core/BaseBuffer.ipp"

#endif // MARLIN_CORE_BASEBUFFER_HPP
