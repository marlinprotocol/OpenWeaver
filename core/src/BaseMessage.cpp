#include "marlin/core/BaseMessage.hpp"


namespace marlin {
namespace core {

BaseMessage BaseMessage::create(size_t size) {
	return BaseMessage { core::Buffer(size) };
}

BaseMessage BaseMessage::create(core::Buffer&& buf) {
	return BaseMessage { std::move(buf) };
}

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
	buf.write_unsafe(0, in, size);

	return std::move(*this);
}

BaseMessage& BaseMessage::set_payload(std::initializer_list<uint8_t> il) & {
	buf.write_unsafe(0, il.begin(), il.size());

	return *this;
}

BaseMessage&& BaseMessage::set_payload(std::initializer_list<uint8_t> il) && {
	buf.write_unsafe(0, il.begin(), il.size());

	return std::move(*this);
}

core::Buffer BaseMessage::finalize() {
	return std::move(buf);
}

core::Buffer BaseMessage::release() {
	return std::move(buf);
}

} // namespace core
} // namespace marlin
