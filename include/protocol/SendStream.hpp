#ifndef STREAM_SEND_STREAM_HPP
#define STREAM_SEND_STREAM_HPP

#include <memory>

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
	uint8_t stream_id;

	enum class State {
		Ready,
		Send,
		Sent,
		Acked
	};
	State state;

	std::unique_ptr<char[]> data;
	size_t size;

	SendStream(uint8_t stream_id, std::unique_ptr<char[]> &&data, size_t size) {
		this->stream_id = stream_id;
		this->data = std::move(data);
		this->size = size;

		this->state = State::Ready;
	}

	uint64_t last_sent_packet = -1;
	std::map<uint64_t, SentPacketInfo> sent_packets;
};

} // namespace stream

#endif // STREAM_SEND_STREAM_HPP
