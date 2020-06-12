/*! \file SendStream.hpp
    \brief components for QUIC like multi-stream implementation over UDP
*/

#ifndef MARLIN_STREAM_SENDSTREAM_HPP
#define MARLIN_STREAM_SENDSTREAM_HPP

#include <memory>
#include <list>
#include <ctime>
#include <map>
#include <marlin/asyncio/core/Timer.hpp>

namespace marlin {
namespace stream {

/// Struct to store data (and its params) which is queued to the output/send stream
struct DataItem {
	/// Data buffer which is to be sent
	core::Buffer data;
	/// Offset in buffer which has already been sent at least once
	uint64_t sent_offset = 0;
	/// Offset of the start of the data buffer in the stream
	uint64_t stream_offset;

	/// Constructor
	DataItem(
		core::Buffer &&_data,
		uint64_t _stream_offset
	) : data(std::move(_data)), stream_offset(_stream_offset) {}
};

struct SendStream;

/// Information about a sent packet
struct SentPacketInfo {
	/// Time it was sent (relative to arbitrary epoch)
	uint64_t sent_time;
	/// Stream it was sent on
	SendStream *stream;
	/// Data item whose data was sent
	DataItem *data_item;
	/// Offset in the data item from which data was went
	uint64_t offset;
	/// Length of the data sent
	uint16_t length;

	/// Constructor
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

	/// Default constructor
	SentPacketInfo() {}

	/// Equality
	bool operator==(SentPacketInfo& other) {
		return this->sent_time == other.sent_time &&
			this->stream == other.stream &&
			this->data_item == other.data_item &&
			this->offset == other.offset &&
			this->length == other.length;
	}

	/// Inequality
	bool operator!=(SentPacketInfo& other) {
		return !(*this == other);
	}
};

/// A send stream which can send queued data
struct SendStream {
	/// Stream id
	uint16_t stream_id;

	/// Stream state
	enum class State {
		Ready,
		Send,
		Sent,
		Acked
	};
	/// Stream state
	State state;

	/// Constructor
	template<typename DelegateType>
	SendStream(uint16_t stream_id, DelegateType* delegate) : state_timer(delegate) {
		this->stream_id = stream_id;
		this->state = State::Ready;

		this->next_item_iterator = this->data_queue.end();

		state_timer.set_data(this);
	}

	/// Data queue which needs to be sent
	std::list<DataItem> data_queue;

	/// Offset of end of queued data in the stream
	uint64_t queue_offset = 0;

	/// Offset of end of sent data in the stream
	uint64_t sent_offset = 0;
	/// Next item which is to be sent
	std::list<DataItem>::iterator next_item_iterator;

	/// Number of bytes sent but not acked
	uint64_t bytes_in_flight = 0;

	/// Is the application done adding data to the queue?
	bool done_queueing = false;

	/// Offset of end of acked data in the stream
	uint64_t acked_offset = 0;
	/// Acks which have not been processed yet, usually due to having unacked data in front
	std::map<uint64_t, uint16_t> outstanding_acks;

	/// Timer interval for the state timer
	uint64_t state_timer_interval = 1000;
	/// Timer to retry SKIPSTREAM
	asyncio::Timer state_timer;
};

} // namespace stream
} // namespace marlin

#endif // MARLIN_STREAM_SENDSTREAM_HPP
