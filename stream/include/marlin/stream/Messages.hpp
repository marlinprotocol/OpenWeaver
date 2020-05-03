#ifndef MARLIN_STREAM_MESSAGES_HPP
#define MARLIN_STREAM_MESSAGES_HPP

#include <marlin/core/Buffer.hpp>


namespace marlin {
namespace stream {

struct DIAL : public core::Buffer {
public:
	DIAL(
		uint32_t src_conn_id,
		uint32_t dst_conn_id,
		uint8_t const* payload,
		size_t size
	) : core::Buffer({0, 3}, 10 + size) {
		this->write_uint32_be_unsafe(2, this->src_conn_id);
		this->write_uint32_be_unsafe(6, this->dst_conn_id);
		this->write_unsafe(10, payload, size);
	}
};

} // namespace stream
} // namespace marlin

#endif // MARLIN_STREAM_MESSAGES_HPP
