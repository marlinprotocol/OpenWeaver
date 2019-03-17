#ifndef MARLIN_STREAM_SENDSTREAM_HPP
#define MARLIN_STREAM_SENDSTREAM_HPP

#include <memory>
#include <uv.h>

namespace marlin {
namespace stream {

struct SentPacketInfo {
	std::time_t sent_time;
	uint64_t offset;
	uint16_t length;

	SentPacketInfo(std::time_t sent_time, uint64_t offset, uint64_t length) {
		this->sent_time = sent_time;
		this->offset = offset;
		this->length = length;
	}

	SentPacketInfo() {}
};

struct SendStream {
	uint16_t stream_id;

	enum class State {
		Ready,
		Send,
		Sent,
		Acked
	};
	State state;

	std::unique_ptr<char[]> data;
	size_t size;

	SendStream(uint16_t stream_id, std::unique_ptr<char[]> &&data, size_t size) {
		this->stream_id = stream_id;
		this->data = std::move(data);
		this->size = size;

		this->state = State::Ready;
		this->last_sent_packet = 0;

		uv_timer_init(uv_default_loop(), &this->timer);
	}

	uint64_t last_sent_packet = 0;
	std::map<uint64_t, SentPacketInfo> sent_packets;

	uv_timer_t timer;
};

} // namespace stream
} // namespace marlin

#endif // MARLIN_STREAM_SENDSTREAM_HPP
