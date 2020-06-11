/*! \file Buffer.hpp
*/

#ifndef MARLIN_CORE_BUFFER_HPP
#define MARLIN_CORE_BUFFER_HPP

#include <stdint.h>
#include <uv.h>
#include <memory>
#include <utility>
#include <optional>
// DONOT REMOVE. FAILS TO COMPILE ON MAC OTHERWISE
#include <array>

#include "WeakBuffer.hpp"

namespace marlin {
namespace core {

//! Marlin byte buffer implementation
class Buffer : public WeakBuffer {
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
};

} // namespace core
} // namespace marlin

#endif // MARLIN_CORE_BUFFER_HPP
