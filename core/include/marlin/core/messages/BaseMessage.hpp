/*! \file BaseMessage.hpp
*/

#ifndef MARLIN_CORE_BASEMESSAGE_HPP
#define MARLIN_CORE_BASEMESSAGE_HPP

#include "marlin/core/Buffer.hpp"

namespace marlin {
namespace core {

struct BaseMessage {
	Buffer buf;

	BaseMessage(size_t size);
	BaseMessage(Buffer&& buf);

	WeakBuffer payload_buffer() const&;
	Buffer payload_buffer() &&;
	uint8_t* payload() const;

	BaseMessage& set_payload(uint8_t const* in, size_t size) &;
	BaseMessage&& set_payload(uint8_t const* in, size_t size) &&;
	BaseMessage& set_payload(std::initializer_list<uint8_t> il) &;
	BaseMessage&& set_payload(std::initializer_list<uint8_t> il) &&;

	BaseMessage& truncate_unsafe(size_t size) &;
	BaseMessage&& truncate_unsafe(size_t size) &&;
};

} // namespace core
} // namespace marlin

#endif // MARLIN_CORE_BASEMESSAGE_HPP
