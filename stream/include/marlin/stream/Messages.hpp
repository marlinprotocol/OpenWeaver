#ifndef MARLIN_STREAM_MESSAGES_HPP
#define MARLIN_STREAM_MESSAGES_HPP

#include <marlin/core/Buffer.hpp>


namespace marlin {
namespace stream {

template<typename BaseMessageType>
struct DIAL {
	BaseMessageType base;

	[[nodiscard]] bool validate(size_t payload_size) const {
		return base.payload_buffer().size() >= 10 + payload_size;
	}

	static DIAL create(size_t payload_size) {
		return DIAL { BaseMessageType::create(10 + payload_size).set_payload({0, 3}) };
	}

	static DIAL create(core::Buffer&& buf) {
		return DIAL { BaseMessageType::create(std::move(buf)) };
	}

	DIAL& set_src_conn_id(uint32_t src_conn_id) & {
		base.payload_buffer().write_uint32_be_unsafe(2, src_conn_id);

		return *this;
	}

	DIAL&& set_src_conn_id(uint32_t src_conn_id) && {
		return std::move(set_src_conn_id(src_conn_id));
	}

	uint32_t src_conn_id() const {
		return base.payload_buffer().read_uint32_be_unsafe(6);
	}

	DIAL& set_dst_conn_id(uint32_t dst_conn_id) & {
		base.payload_buffer().write_uint32_be_unsafe(6, dst_conn_id);

		return *this;
	}

	DIAL&& set_dst_conn_id(uint32_t dst_conn_id) && {
		return std::move(set_dst_conn_id(dst_conn_id));
	}

	uint32_t dst_conn_id() const {
		return base.payload_buffer().read_uint32_be_unsafe(2);
	}

	DIAL& set_payload(uint8_t const* in, size_t size) & {
		base.payload_buffer().write_unsafe(10, in, size);

		return *this;
	}

	DIAL&& set_payload(uint8_t const* in, size_t size) && {
		return std::move(set_payload(in, size));
	}

	DIAL& set_payload(std::initializer_list<uint8_t> il) & {
		base.payload_buffer().write_unsafe(10, il.begin(), il.size());

		return *this;
	}

	DIAL&& set_payload(std::initializer_list<uint8_t> il) && {
		return std::move(set_payload(il));
	}

	uint8_t* payload() {
		return base.payload_buffer().data() + 10;
	}

	core::Buffer finalize() {
		return base.finalize();
	}

	core::Buffer release() {
		return base.release();
	}
};

struct DIALCONF : public core::Buffer {
public:
	DIALCONF(
		uint32_t src_conn_id,
		uint32_t dst_conn_id,
		uint8_t const* payload,
		size_t payload_size
	) : core::Buffer({0, 4}, 10 + payload_size) {
		this->write_uint32_be_unsafe(2, src_conn_id);
		this->write_uint32_be_unsafe(6, dst_conn_id);
		this->write_unsafe(10, payload, payload_size);
	}

	DIALCONF(core::Buffer&& buf) : core::Buffer(std::move(buf)) {}

	[[nodiscard]] bool validate(size_t payload_size) const {
		return this->size() >= 10 + payload_size;
	}

	uint32_t src_conn_id() const {
		return this->read_uint32_be_unsafe(6);
	}

	uint32_t dst_conn_id() const {
		return this->read_uint32_be_unsafe(2);
	}

	uint8_t* payload() {
		return this->data() + 10;
	}
};

struct CONF : public core::Buffer {
public:
	CONF(
		uint32_t src_conn_id,
		uint32_t dst_conn_id
	) : core::Buffer({0, 5}, 10) {
		this->write_uint32_be_unsafe(2, src_conn_id);
		this->write_uint32_be_unsafe(6, dst_conn_id);
	}

	CONF(core::Buffer&& buf) : core::Buffer(std::move(buf)) {}

	[[nodiscard]] bool validate() const {
		return this->size() >= 10;
	}

	uint32_t src_conn_id() const {
		return this->read_uint32_be_unsafe(6);
	}

	uint32_t dst_conn_id() const {
		return this->read_uint32_be_unsafe(2);
	}
};

struct RST : public core::Buffer {
public:
	RST(
		uint32_t src_conn_id,
		uint32_t dst_conn_id
	) : core::Buffer({0, 6}, 10) {
		this->write_uint32_be_unsafe(2, src_conn_id);
		this->write_uint32_be_unsafe(6, dst_conn_id);
	}

	RST(core::Buffer&& buf) : core::Buffer(std::move(buf)) {}

	[[nodiscard]] bool validate() const {
		return this->size() >= 10;
	}

	uint32_t src_conn_id() const {
		return this->read_uint32_be_unsafe(6);
	}

	uint32_t dst_conn_id() const {
		return this->read_uint32_be_unsafe(2);
	}
};

} // namespace stream
} // namespace marlin

#endif // MARLIN_STREAM_MESSAGES_HPP
