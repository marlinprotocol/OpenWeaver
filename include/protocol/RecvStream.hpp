#ifndef MARLIN_STREAM_RECVSTREAM_HPP
#define MARLIN_STREAM_RECVSTREAM_HPP

#include "StreamPacket.hpp"

#include <ctime>
#include <memory>

namespace marlin {
namespace stream {

struct RecvPacketInfo {
	std::time_t recv_time;
	uint64_t offset;
	uint16_t length;
	net::Packet packet;

	RecvPacketInfo(
		std::time_t recv_time,
		uint64_t offset,
		uint64_t length,
		net::Packet &&_packet
	) : packet(std::move(_packet)) {
		this->recv_time = recv_time;
		this->offset = offset;
		this->length = length;
	}

	RecvPacketInfo() : packet(nullptr, 0) {}

	RecvPacketInfo(const RecvPacketInfo &) = delete;

	RecvPacketInfo(RecvPacketInfo &&info) : packet(std::move(info.packet)) {
		this->recv_time = info.recv_time;
		this->offset = info.offset;
		this->length = info.length;
	};

	RecvPacketInfo &operator=(RecvPacketInfo &&info) {
		this->recv_time = info.recv_time;
		this->offset = info.offset;
		this->length = info.length;
		this->packet = std::move(info.packet);

		return *this;
	};
};

template<typename RecvDelegate>
struct RecvStream {
	uint16_t stream_id;
	RecvDelegate &delegate;

	enum class State {
		Recv,
		SizeKnown,
		AllRecv,
		Read
	};
	State state;

	uint64_t size = 0;

	RecvStream(uint16_t stream_id, RecvDelegate &_delegate) : delegate(_delegate) {
		this->stream_id = stream_id;
		this->state = State::Recv;
	}

	std::map<uint64_t, RecvPacketInfo> recv_packets;

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

	uint64_t read_offset = 0;

	bool check_read() const {
		if (this->state == State::Recv) {
			return false;
		}

		return this->read_offset == this->size;
	}

};

} // namespace stream
} // namespace marlin

#endif // MARLIN_STREAM_RECVSTREAM_HPP
