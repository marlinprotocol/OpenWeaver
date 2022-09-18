/*! \file Buffer.hpp
*/

#ifndef MARLIN_CORE_BUFFER_HPP
#define MARLIN_CORE_BUFFER_HPP

#include "marlin/core/WeakBuffer.hpp"

namespace marlin {
namespace core {

/// @brief Byte buffer implementation with modifiable bounds and memory ownership
/// @headerfile Buffer.hpp <marlin/core/Buffer.hpp>
class [[clang::trivial_abi]] Buffer : public BaseBuffer<Buffer> {
public:
	using BaseBuffer<Buffer>::BaseBuffer;

	/// Construct with given size - preferred constructor
	Buffer(size_t size);

	/// Construct with initializer list and given size - preferred constructor
	Buffer(std::initializer_list<uint8_t> il, size_t size);
	// Initializer lists are preferable to partially initialized uint8_t arrays
	// Buffer(new uint8_t[10] {'0','1'}, 10) causes zeroing of remaining 8 bytes
	// Buffer({'0','1'}, 10) doesn't

	/// Construct from uint8_t array - unsafe if uint8_t * isn't obtained from new
	Buffer(uint8_t *buf, size_t size);

	/// Move contructor
	Buffer(Buffer &&b) noexcept;

	/// Delete copy contructor
	Buffer(Buffer const &b) = delete;

	/// Move assign
	Buffer &operator=(Buffer &&b) noexcept;

	/// Delete copy assign
	Buffer &operator=(Buffer const &p) = delete;

	~Buffer();

	/// Implicit conversion to WeakBuffer
	operator WeakBuffer() {
		return WeakBuffer(data(), size());
	}

	operator WeakBuffer const() const {
		// Note: Const stripping, but safe since return value is const
		return WeakBuffer((uint8_t*)data(), size());
	}

	/// Release the memory held by the buffer
	inline uint8_t *release() {
		uint8_t *_buf = buf;

		buf = nullptr;
		capacity = 0;
		start_index = 0;
		end_index = 0;

		return _buf;
	}

	/// Get a WeakBuffer corresponding to the payload area
	WeakBuffer payload_buffer() &;
	WeakBuffer const payload_buffer() const&;
	/// Get a Buffer corresponding to the payload area, consumes the existing object
	Buffer payload_buffer() &&;
	/// Get a uint8_t* corresponding to the payload area
	uint8_t* payload();
	uint8_t const* payload() const;
};

} // namespace core
} // namespace marlin

#endif // MARLIN_CORE_BUFFER_HPP
