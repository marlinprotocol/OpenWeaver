#ifndef MARLIN_CORE_BASEMESSAGE_HPP
#define MARLIN_CORE_BASEMESSAGE_HPP

#include "marlin/core/Buffer.hpp"

namespace marlin {
namespace core {

/// @brief Base message type with a payload
/// @headerfile BaseMessage.hpp <marlin/core/BaseMessage.hpp>
struct BaseMessage {
	/// Underlying buffer
	Buffer buf;

	/// Construct from size
	BaseMessage(size_t size);
	/// Construct from a buffer
	BaseMessage(Buffer&& buf);

	/// Get a WeakBuffer corresponding to the payload area
	WeakBuffer payload_buffer() const&;
	/// Get a Buffer corresponding to the payload area, consumes the BaseMessage object
	Buffer payload_buffer() &&;
	/// Get a uint8_t* corresponding to the payload area
	uint8_t* payload() const;

	/// @{
	/// Sets the payload given a byte address and size
	BaseMessage& set_payload(uint8_t const* in, size_t size) &;
	BaseMessage&& set_payload(uint8_t const* in, size_t size) &&;
	/// @}
	/// @{
	/// Sets the payload given a initializer list with bytes
	BaseMessage& set_payload(std::initializer_list<uint8_t> il) &;
	BaseMessage&& set_payload(std::initializer_list<uint8_t> il) &&;
	/// @}

	/// @{
	/// Truncates the underlying buffer to the given size
	BaseMessage& truncate_unsafe(size_t size) &;
	BaseMessage&& truncate_unsafe(size_t size) &&;
	/// @}
};

} // namespace core
} // namespace marlin

#endif // MARLIN_CORE_BASEMESSAGE_HPP
