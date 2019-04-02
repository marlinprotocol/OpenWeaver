#ifndef MARLIN_STREAM_SENDSTREAM_HPP
#define MARLIN_STREAM_SENDSTREAM_HPP

#include <memory>
#include <uv.h>
#include <list>
#include <ctime>
#include <map>

namespace marlin {
namespace stream {

struct DataItem {
	std::unique_ptr<char[]> data;
	uint64_t size;
	uint64_t sent_offset = 0;
	uint64_t stream_offset;

	DataItem(
		std::unique_ptr<char[]> &&_data,
		uint64_t _size,
		uint64_t _stream_offset
	) : data(std::move(_data)), size(_size), stream_offset(_stream_offset) {}
};

struct SentPacketInfo {
	std::time_t sent_time;
	DataItem *data_item;
	uint64_t offset;
	uint16_t length;

	SentPacketInfo(
		std::time_t sent_time,
		DataItem *data_item,
		uint64_t offset,
		uint64_t length
	) {
		this->sent_time = sent_time;
		this->data_item = data_item;
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

	SendStream(uint16_t stream_id) {
		this->stream_id = stream_id;
		this->state = State::Ready;

		this->next_item_iterator = this->data_queue.end();

		uv_timer_init(uv_default_loop(), &this->timer);
	}

	std::list<DataItem> data_queue;
	uint64_t queue_offset = 0;

	uint64_t last_sent_packet = -1;
	std::map<uint64_t, SentPacketInfo> sent_packets;
	uint64_t sent_offset = 0;
	std::list<DataItem>::iterator next_item_iterator;

	uint64_t bytes_in_flight = 0;
	uint64_t congestion_window = 80000;
	// uint64_t ssthresh = -1;

	bool done_queueing = false;

	uv_timer_t timer;
};

} // namespace stream
} // namespace marlin

#endif // MARLIN_STREAM_SENDSTREAM_HPP
