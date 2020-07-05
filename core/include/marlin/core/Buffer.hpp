/*! \file Buffer.hpp
*/

#ifndef MARLIN_CORE_BUFFER_HPP
#define MARLIN_CORE_BUFFER_HPP

#include "marlin/core/WeakBuffer.hpp"

namespace marlin {
namespace core {

/// @brief Byte buffer implementation with modifiable bounds and memory ownership
/// @headerfile Buffer.hpp <marlin/core/Buffer.hpp>
class Buffer : public BaseBuffer<Buffer> {
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

	/// Release the memory held by the buffer
	inline uint8_t *release() {
		uint8_t *_buf = buf;

		buf = nullptr;
		capacity = 0;
		start_index = 0;
		end_index = 0;

		return _buf;
	}
};

} // namespace core
} // namespace marlin

#endif // MARLIN_CORE_BUFFER_HPP
