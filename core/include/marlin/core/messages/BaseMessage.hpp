/*! \file BaseMessage.hpp
*/

#ifndef MARLIN_CORE_BASEMESSAGE_HPP
#define MARLIN_CORE_BASEMESSAGE_HPP

#include "marlin/core/Buffer.hpp"

namespace marlin {
namespace core {

struct BaseMessage {
	core::Buffer buf;

	static BaseMessage create(size_t size);
	static BaseMessage create(core::Buffer&& buf);

	core::WeakBuffer payload_buffer() const;
	uint8_t* payload() const;

	BaseMessage& set_payload(uint8_t const* in, size_t size) &;
	BaseMessage&& set_payload(uint8_t const* in, size_t size) &&;
	BaseMessage& set_payload(std::initializer_list<uint8_t> il) &;
	BaseMessage&& set_payload(std::initializer_list<uint8_t> il) &&;

	BaseMessage& truncate_unsafe(size_t size) &;
	BaseMessage&& truncate_unsafe(size_t size) &&;

	core::Buffer finalize();
	core::Buffer release();
};

} // namespace core
} // namespace marlin

#endif // MARLIN_CORE_BASEMESSAGE_HPP
