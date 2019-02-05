#ifndef STREAM_RECEIVE_STREAM_HPP
#define STREAM_RECEIVE_STREAM_HPP

#include <ctime>
#include <memory>

namespace stream {

struct RecvPacketInfo {
	std::time_t recv_time;
	uint64_t offset;
	uint16_t length;
	std::unique_ptr<char[]> data;

	RecvPacketInfo(
		std::time_t recv_time,
		uint64_t offset,
		uint64_t length,
		std::unique_ptr<char[]> &&_data
	) : data(std::move(_data)) {
		this->recv_time = recv_time;
		this->offset = offset;
		this->length = length;
	}

	RecvPacketInfo() {}

	RecvPacketInfo(const RecvPacketInfo &) = delete;

	RecvPacketInfo(RecvPacketInfo &&info) : data(std::move(info.data)) {
		this->recv_time = info.recv_time;
		this->offset = info.offset;
		this->length = info.length;
	};

	RecvPacketInfo &operator=(RecvPacketInfo &&info) {
		this->recv_time = info.recv_time;
		this->offset = info.offset;
		this->length = info.length;
		this->data = std::move(info.data);
	};
};

template<typename RecvDelegate>
struct RecvStream {
	uint8_t stream_id;
	RecvDelegate &delegate;

	enum class State {
		Recv,
		SizeKnown,
		AllRecv,
		Read
	};
	State state;

	uint64_t size = 0;

	RecvStream(uint8_t stream_id, RecvDelegate &_delegate) : delegate(_delegate) {
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

		uint64_t offset = 0;  // Expected offset
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
};

} // namespace stream

#endif // STREAM_RECEIVE_STREAM_HPP
