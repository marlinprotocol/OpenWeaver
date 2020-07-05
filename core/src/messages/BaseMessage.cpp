#include "marlin/core/messages/BaseMessage.hpp"


namespace marlin {
namespace core {

BaseMessage::BaseMessage(size_t size) : buf(size) {}

BaseMessage::BaseMessage(Buffer&& buf) : buf(std::move(buf)) {}

WeakBuffer BaseMessage::payload_buffer() & {
	return buf;
}

WeakBuffer const BaseMessage::payload_buffer() const& {
	return buf;
}

Buffer BaseMessage::payload_buffer() && {
	return std::move(buf);
}

uint8_t* BaseMessage::payload() {
	return buf.data();
}

uint8_t const* BaseMessage::payload() const {
	return buf.data();
}

BaseMessage& BaseMessage::set_payload(uint8_t const* in, size_t size) & {
	buf.write_unsafe(0, in, size);

	return *this;
}

BaseMessage&& BaseMessage::set_payload(uint8_t const* in, size_t size) && {
	return std::move(set_payload(in, size));
}

BaseMessage& BaseMessage::set_payload(std::initializer_list<uint8_t> il) & {
	buf.write_unsafe(0, il.begin(), il.size());

	return *this;
}

BaseMessage&& BaseMessage::set_payload(std::initializer_list<uint8_t> il) && {
	return std::move(set_payload(il));
}

BaseMessage& BaseMessage::truncate_unsafe(size_t size) & {
	buf.truncate_unsafe(size);

	return *this;
}

BaseMessage&& BaseMessage::truncate_unsafe(size_t size) && {
	return std::move(truncate_unsafe(size));
}

} // namespace core
} // namespace marlin
