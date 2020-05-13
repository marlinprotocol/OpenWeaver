#include "marlin/core/messages/BaseMessage.hpp"


namespace marlin {
namespace core {

BaseMessage::BaseMessage(size_t size) : buf(size) {}

BaseMessage::BaseMessage(core::Buffer&& buf) : buf(std::move(buf)) {}

core::WeakBuffer BaseMessage::payload_buffer() const {
	return buf;
}

uint8_t* BaseMessage::payload() const {
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
	// TODO: This works, but is this actually correct?
	return std::move(set_payload(il));
}

BaseMessage& BaseMessage::truncate_unsafe(size_t size) & {
	buf.truncate_unsafe(size);

	return *this;
}

BaseMessage&& BaseMessage::truncate_unsafe(size_t size) && {
	// TODO: This works, but is this actually correct?
	return std::move(truncate_unsafe(size));
}

core::Buffer BaseMessage::finalize() {
	return std::move(buf);
}

core::Buffer BaseMessage::release() {
	return std::move(buf);
}

} // namespace core
} // namespace marlin
