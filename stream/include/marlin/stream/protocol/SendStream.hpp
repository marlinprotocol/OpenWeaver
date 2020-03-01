/*! \file SendStream.hpp
    \brief components for QUIC like multi-stream implementation over UDP
*/

#ifndef MARLIN_STREAM_SENDSTREAM_HPP
#define MARLIN_STREAM_SENDSTREAM_HPP

#include <memory>
#include <uv.h>
#include <list>
#include <ctime>
#include <map>

namespace marlin {
namespace stream {

//! Struct to store data (and its params) which is queued to the output/send stream
struct DataItem {
	net::Buffer data;
	uint64_t sent_offset = 0;
	uint64_t stream_offset;

	DataItem(
		net::Buffer &&_data,
		uint64_t _stream_offset
	) : data(std::move(_data)), stream_offset(_stream_offset) {}
};

struct SendStream;

struct SentPacketInfo {
	uint64_t sent_time;
	SendStream *stream;
	DataItem *data_item;
	uint64_t offset;
	uint16_t length;

	SentPacketInfo(
		uint64_t sent_time,
		SendStream *stream,
		DataItem *data_item,
		uint64_t offset,
		uint64_t length
	) {
		this->sent_time = sent_time;
		this->stream = stream;
		this->data_item = data_item;
		this->offset = offset;
		this->length = length;
	}

	SentPacketInfo() {}

	bool operator==(SentPacketInfo& other) {
		return this->sent_time == other.sent_time &&
			this->stream == other.stream &&
			this->data_item == other.data_item &&
			this->offset == other.offset &&
			this->length == other.length;
	}

	bool operator!=(SentPacketInfo& other) {
		return !(*this == other);
	}
};

//! Implementation for individual stream in multi-stream transport protocol
/*!
	Features:
	\li has its own congestion control
	\li is responsible for ensuring the packet delivery of its dataItems
*/
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

		uv_timer_init(uv_default_loop(), &state_timer);
		state_timer.data = nullptr;
	}

	std::list<DataItem> data_queue;

	/*!
		Represents the total number of bytes added to the queue
	*/
	uint64_t queue_offset = 0;

	uint64_t sent_offset = 0;
	std::list<DataItem>::iterator next_item_iterator;

	uint64_t bytes_in_flight = 0;

	bool done_queueing = false;

	uint64_t acked_offset = 0;
	std::map<uint64_t, uint16_t> outstanding_acks;

	uint64_t state_timer_interval = 1000;
	uv_timer_t state_timer;
};

} // namespace stream
} // namespace marlin

#endif // MARLIN_STREAM_SENDSTREAM_HPP
