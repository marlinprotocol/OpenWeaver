#ifndef MARLIN_STREAM_RECVSTREAM_HPP
#define MARLIN_STREAM_RECVSTREAM_HPP

#include <marlin/asyncio/core/Timer.hpp>

#include <ctime>
#include <memory>

namespace marlin {
namespace stream {

/// Store a received packet with a few header fields
struct RecvPacketInfo {
	/// Time it was received (relative to arbitrary epoch)
	uint64_t recv_time;
	/// Offset of data in stream
	uint64_t offset;
	/// Length of length
	uint16_t length;
	/// Data that was received
	core::Buffer packet;

	/// Constructor
	RecvPacketInfo(
		uint64_t recv_time,
		uint64_t offset,
		uint64_t length,
		core::Buffer &&_packet
	) : packet(std::move(_packet)) {
		this->recv_time = recv_time;
		this->offset = offset;
		this->length = length;
	}

	/// Default constructor
	RecvPacketInfo() : packet(nullptr, 0) {
		recv_time = 0;
		offset = 0;
		length = 0;
	}

	/// Delete copy constructor
	RecvPacketInfo(const RecvPacketInfo &) = delete;

	/// Move constructor
	RecvPacketInfo(RecvPacketInfo &&info) : packet(std::move(info.packet)) {
		this->recv_time = info.recv_time;
		this->offset = info.offset;
		this->length = info.length;
	};

	/// Move assign
	RecvPacketInfo &operator=(RecvPacketInfo &&info) {
		this->recv_time = info.recv_time;
		this->offset = info.offset;
		this->length = info.length;
		this->packet = std::move(info.packet);

		return *this;
	};
};

/// A recv stream which handles incoming data
struct RecvStream {
	/// Stream id
	uint16_t stream_id;

	/// Stream state
	enum class State {
		Recv,
		SizeKnown,
		AllRecv,
		Read
	};
	/// Stream state
	State state;
	/// Is it waiting for the destination to flush the stream?
	bool wait_flush = false;

	/// Total size of the stream
	uint64_t size = 0;

	/// Constructor
	template<typename DelegateType>
	RecvStream(uint16_t stream_id, DelegateType* delegate) : state_timer(delegate) {
		this->stream_id = stream_id;
		this->state = State::Recv;

		state_timer.set_data(this);
	}

	/// List of received packets
	std::map<uint64_t, RecvPacketInfo> recv_packets;

	/// Check if entire data on stream has been received
	bool check_finish() const {
		if (this->state == State::Recv) {
			return false;
		}
		if (this->state == State::AllRecv || this->state == State::Read) {
			return true;
		}

		uint64_t offset = read_offset;  // Expected offset
		for (auto iter = this->recv_packets.begin(); iter != this->recv_packets.end(); iter++) {
			// Packet should start at or before expected offset
			if (offset < iter->second.offset) {
				return false;
			}

			// Update expected offset
			offset = std::max(offset, iter->second.offset + iter->second.length);
		}

		return offset == this->size;
	}

	/// Offset marking application read position on the stream
	uint64_t read_offset = 0;

	/// Check if all data on stream has been read by application
	bool check_read() const {
		if (this->state == State::Recv) {
			return false;
		}

		return this->read_offset == this->size;
	}

	/// Timer interval for the state timer
	uint64_t state_timer_interval = 1000;
	/// Timer to retry SKIPSTREAM
	asyncio::Timer state_timer;
};

} // namespace stream
} // namespace marlin

#endif // MARLIN_STREAM_RECVSTREAM_HPP
