#ifndef MARLIN_STREAM_CONNECTION_HPP
#define MARLIN_STREAM_CONNECTION_HPP

#include "SendStream.hpp"
#include "RecvStream.hpp"

#include <unordered_set>

namespace marlin {
namespace stream {

template<typename NodeType>
class StreamProtocol;

constexpr uint32_t const DEFAULT_TLP_INTERVAL = 25;

template<typename Transport, typename RecvDelegate>
class Connection {
public:
	// enum State {
	// 	Open,
	// 	Closed
	// };
	// State state;

	net::SocketAddress addr;
	Transport &transport;

	Connection(net::SocketAddress const &addr, Transport &transport);

	// Streams
	uint16_t next_stream_id = 0;
	std::unordered_map<uint16_t, SendStream> send_streams;
	std::unordered_map<uint16_t, RecvStream<RecvDelegate>> recv_streams;

	SendStream &get_or_create_send_stream(uint16_t const stream_id);
	RecvStream<RecvDelegate> &get_or_create_recv_stream(uint16_t const stream_id, RecvDelegate &delegate);

	std::unordered_set<uint16_t> send_queue_ids;
	std::list<SendStream *> send_queue;

	bool register_send_intent(SendStream &stream);

	// Packets
	uint64_t last_sent_packet = -1;
	std::map<uint64_t, SentPacketInfo> sent_packets;

	// Congestion, flow control
	uint64_t bytes_in_flight = 0;
	uint64_t congestion_window = 80000;

	void process_pending_data();
	int process_pending_data(SendStream &stream);

	// Tail loss timer
	uv_timer_t timer;
	uint32_t timer_interval = DEFAULT_TLP_INTERVAL;

	void _send_data_packet(
		const net::SocketAddress &addr,
		SendStream &stream,
		DataItem &data_item,
		uint64_t offset,
		uint16_t length
	);
};


// Impl

template<typename Transport, typename RecvDelegate>
Connection<Transport, RecvDelegate>::Connection(
	net::SocketAddress const &_addr,
	Transport &_transport
) : addr(_addr), transport(_transport) {
	uv_timer_init(uv_default_loop(), &this->timer);

	this->timer.data = this;
}

template<typename Transport, typename RecvDelegate>
SendStream &Connection<Transport, RecvDelegate>::get_or_create_send_stream(uint16_t const stream_id) {
	auto iter = send_streams.try_emplace(
		stream_id,
		stream_id
	).first;

	return iter->second;
}

template<typename Transport, typename RecvDelegate>
RecvStream<RecvDelegate> &Connection<Transport, RecvDelegate>::get_or_create_recv_stream(uint16_t const stream_id, RecvDelegate &delegate) {
	auto iter = recv_streams.try_emplace(
		stream_id,
		stream_id,
		delegate
	).first;

	return iter->second;
}

template<typename Transport, typename RecvDelegate>
bool Connection<Transport, RecvDelegate>::register_send_intent(SendStream &stream) {
	if(send_queue_ids.find(stream.stream_id) != send_queue_ids.end()) {
		return false;
	}

	send_queue.push_back(&stream);
	send_queue_ids.insert(stream.stream_id);

	return true;
}

constexpr uint16_t const DEFAULT_FRAGMENT_SIZE = 1000;

template<typename Transport, typename RecvDelegate>
void Connection<Transport, RecvDelegate>::process_pending_data() {
	// TODO: Better scheduler
	for(
		auto iter = send_queue.begin();
		iter != send_queue.end();
		// Empty
	) {
		auto &stream = **iter;

		int res = process_pending_data(stream);
		if(res == 0) { // Idle stream, move to next stream
			send_queue_ids.erase(stream.stream_id);
			iter = send_queue.erase(iter);
		} else if(res == -1) { // Stream window exhausted, move to next stream
			iter++;
		} else { // Connection window exhausted, break
			break;
		}
	}
}

template<typename Transport, typename RecvDelegate>
int Connection<Transport, RecvDelegate>::process_pending_data(SendStream &stream) {
	for(
		;
		stream.next_item_iterator != stream.data_queue.end();
		stream.next_item_iterator++
	) {
		auto &data_item = *stream.next_item_iterator;

		for(
			uint64_t i = data_item.sent_offset;
			i < data_item.size;
			i+=DEFAULT_FRAGMENT_SIZE
		) {
			auto remaining_bytes = data_item.size - data_item.sent_offset;
			uint16_t dsize = remaining_bytes > DEFAULT_FRAGMENT_SIZE ? DEFAULT_FRAGMENT_SIZE : remaining_bytes;

			if(stream.bytes_in_flight > stream.congestion_window - dsize)
				return -1;
			if(this->bytes_in_flight > this->congestion_window - dsize)
				return -2;

			_send_data_packet(addr, stream, data_item, i, dsize);

			stream.bytes_in_flight += dsize;
			stream.sent_offset += dsize;
			this->bytes_in_flight += dsize;
			data_item.sent_offset += dsize;
		}
	}

	return 0;
}


template<typename Transport, typename RecvDelegate>
void Connection<Transport, RecvDelegate>::_send_data_packet(
	const net::SocketAddress &addr,
	SendStream &stream,
	DataItem &data_item,
	uint64_t offset,
	uint16_t length
) {
	bool is_fin = (stream.done_queueing &&
		data_item.stream_offset + offset + length >= stream.queue_offset);

	char *pdata = new char[1100] {0, static_cast<char>(is_fin)};

	uint16_t n_stream_id = htons(stream.stream_id);
	std::memcpy(pdata+2, &n_stream_id, 2);
	uint64_t n_packet_number = htonll(this->last_sent_packet+1);
	std::memcpy(pdata+4, &n_packet_number, 8);

	uint64_t n_offset = htonll(data_item.stream_offset + offset);
	std::memcpy(pdata+12, &n_offset, 8);

	uint16_t n_length = htons(length);
	std::memcpy(pdata+20, &n_length, 2);

	std::memcpy(pdata+22, data_item.data.get()+offset, length);

	this->last_sent_packet++;
	this->sent_packets.emplace(
		std::piecewise_construct,
		std::forward_as_tuple(this->last_sent_packet),
		std::forward_as_tuple(
			std::time(nullptr),
			&stream,
			&data_item,
			offset,
			length
		)
	);

	net::Packet p(pdata, length+22);
	StreamProtocol<Transport>::send_DATA(transport, addr, std::move(p));

	if(is_fin && stream.state != SendStream::State::Acked) {
		stream.state = SendStream::State::Sent;
	}
}


} // namespace stream
} // namespace marlin

#endif // MARLIN_STREAM_CONNECTION_HPP
