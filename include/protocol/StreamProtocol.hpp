#ifndef MARLIN_STREAM_STREAMPROTOCOL_HPP
#define MARLIN_STREAM_STREAMPROTOCOL_HPP

#include <marlin/net/SocketAddress.hpp>
#include "StreamPacket.hpp"
#include "SendStream.hpp"
#include "RecvStream.hpp"
#include "Connection.hpp"

#include <cstring>
#include <algorithm>

#include <spdlog/spdlog.h>

#include <iostream>

namespace marlin {
namespace stream {

template<typename NodeType>
using StreamStorage = std::map<net::SocketAddress, Connection<NodeType>>;

template<typename NodeType>
class StreamProtocol {
public:
	static void setup(NodeType &) {}

	static void did_receive_packet(
		NodeType &node,
		net::Packet &&p,
		const net::SocketAddress &addr
	) {
		auto pp = reinterpret_cast<StreamPacket *>(&p);
		switch(pp->message()) {
			// DATA, DATA + FIN
			case 0:
			case 1: did_receive_DATA(node, addr, std::move(*pp));
			break;
			// ACK
			case 2: did_receive_ACK(node, addr, std::move(*pp));
			break;
			// UNKNOWN
			default: spdlog::debug("UNKNOWN <<< {}", addr.to_string());
			break;
		}
	}

	static void did_send_packet(
		NodeType &,
		net::Packet &&p,
		const net::SocketAddress &addr
	) {
		auto pp = reinterpret_cast<StreamPacket *>(&p);
		switch(pp->message()) {
			// DATA
			case 0: spdlog::debug("DATA >>> {}", addr.to_string());;
			break;
			// DATA + FIN
			case 1: spdlog::debug("DATA + FIN >>> {}", addr.to_string());;
			break;
			// ACK
			case 2: spdlog::debug("ACK >>> {}", addr.to_string());;
			break;
			// UNKNOWN
			default: spdlog::debug("UNKNOWN >>> {}", addr.to_string());
			break;
		}
	}

	// New stream
	static void send_data(NodeType &node, std::unique_ptr<char[]> &&data, uint64_t size, const net::SocketAddress &addr) {
		auto &conn = node.stream_storage[addr];

		send_data(node, conn.next_stream_id, std::move(data), size, addr);

		conn.next_stream_id += 2;
	}

	// Explicit stream id. Stream created if not found.
	static void send_data(NodeType &node, uint16_t stream_id, std::unique_ptr<char[]> &&data, uint64_t size, const net::SocketAddress &addr) {
		auto &conn = node.stream_storage[addr];

		auto stream_iter = conn.send_streams.find(stream_id);
		if(stream_iter == conn.send_streams.end()) {
			// Create stream
			stream_iter = conn.send_streams.emplace(
				std::piecewise_construct,
				std::forward_as_tuple(stream_id),
				std::forward_as_tuple(stream_id)
			).first;

			auto &stream = stream_iter->second;
			stream.state = SendStream::State::Send;

			// Set tail loss recovery timer
			stream.timer.data = new std::tuple<net::SocketAddress, NodeType &, SendStream &>(addr, node, stream);
			uv_timer_start(&stream.timer, &StreamProtocol<NodeType>::timer_cb, 25, 25);
		}

		auto &stream = stream_iter->second;

		// Add data to send queue
		auto new_iter = stream.data_queue.insert(
			stream.data_queue.end(),
			std::move(DataItem(std::move(data), size, stream.queue_offset))
		);

		stream.queue_offset += size;

		// Handle idle stream
		if(stream.next_item_iterator == stream.data_queue.end()) {
			stream.next_item_iterator = new_iter;
			_send_data(node, addr, stream);
		}

		// Note: Don't need to explicitly call _send_data if stream is not idle.
		// It should eventually be called from elsewhere(timer, ack, etc)
	}

	static void did_receive_DATA(NodeType &node, const net::SocketAddress &addr, StreamPacket &&p);
	static void send_DATA(NodeType &node, const net::SocketAddress &addr, net::Packet &&p);

	static void did_receive_ACK(NodeType &node, const net::SocketAddress &addr, StreamPacket &&p);
	static void send_ACK(NodeType &node, const net::SocketAddress &addr, uint16_t stream_id, uint64_t packet_number);

	static void timer_cb(uv_timer_t *handle) {
		auto &tuple = *(std::tuple<net::SocketAddress, NodeType &, SendStream &> *)handle->data;
		auto &node = std::get<1>(tuple);
		auto addr = std::get<0>(tuple);
		auto &stream = std::get<2>(tuple);

		auto sent_iter = stream.sent_packets.cbegin();
		auto num = stream.sent_packets.size();

		// Retry lost packets
		// No condition necessary, all packets considered lost
		// if tail probe fails
		for(size_t i = 0; i < num; i++) {
			// spdlog::debug("{}, {}, {}", sent_iter->first, stream.sent_packets.size(), 0);

			auto &sent_packet = sent_iter->second;

			StreamProtocol<NodeType>::send_data_packet(node, addr, stream, *(sent_packet.data_item), sent_packet.offset, sent_packet.length);

			sent_iter = stream.sent_packets.erase(sent_iter);
		}

		// New packets
		_send_data(node, addr, stream);

		spdlog::debug("Timer End");
	}

private:
	// Create and send packets till congestion control limit
	static void _send_data(
		NodeType &node,
		const net::SocketAddress &addr,
		SendStream &stream
	) {
		for(; stream.next_item_iterator != stream.data_queue.end(); stream.next_item_iterator++) {

			DataItem &data_item = *stream.next_item_iterator;

			for(uint64_t i = data_item.sent_offset; i < data_item.size; i+=1000) {
				bool is_fin = (data_item.sent_offset + 1000 >= data_item.size);
				uint16_t dsize = is_fin ? data_item.size - data_item.sent_offset : 1000;

				if(stream.bytes_in_flight > stream.congestion_window - dsize)
					return;

				StreamProtocol<NodeType>::send_data_packet(node, addr, stream, data_item, i, dsize);

				stream.bytes_in_flight += dsize;
				stream.sent_offset += dsize;
				data_item.sent_offset += dsize;
			}
		}
	}

	static void send_data_packet(
		NodeType &node,
		const net::SocketAddress &addr,
		SendStream &stream,
		DataItem &data_item,
		uint64_t offset,
		uint16_t length
	) {
		bool is_fin = (stream.done_queueing && data_item.stream_offset + offset + length >= stream.queue_offset);

		char *pdata = new char[1100] {0, static_cast<char>(is_fin)};

		uint16_t n_stream_id = htons(stream.stream_id);
		std::memcpy(pdata+2, &n_stream_id, 2);

		uint64_t n_packet_number = htonll(stream.last_sent_packet+1);
		std::memcpy(pdata+4, &n_packet_number, 8);

		uint64_t n_offset = htonll(data_item.stream_offset + offset);
		std::memcpy(pdata+12, &n_offset, 8);

		uint16_t n_length = htons(length);
		std::memcpy(pdata+20, &n_length, 2);

		std::memcpy(pdata+22, data_item.data.get()+offset, length);

		stream.sent_packets[stream.last_sent_packet+1] = SentPacketInfo(std::time(nullptr), &data_item, offset, length);
		stream.last_sent_packet++;

		net::Packet p(pdata, length+22);
		StreamProtocol<NodeType>::send_DATA(node, addr, std::move(p));

		if(is_fin && stream.state != SendStream::State::Acked) {
			stream.state = SendStream::State::Sent;
		}
	}
};

template<typename NodeType>
void StreamProtocol<NodeType>::send_DATA(NodeType &node, const net::SocketAddress &addr, net::Packet &&p) {
	node.send(std::move(p), addr);
}

template<typename NodeType>
void StreamProtocol<NodeType>::did_receive_DATA(NodeType &node, const net::SocketAddress &addr, StreamPacket &&p) {
	spdlog::debug("DATA <<< {}", addr.to_string());

	auto &storage = node.stream_storage[addr];

	if(storage.next_stream_id == 0) {
		storage.next_stream_id = 1;
	}

	// Create stream if it does not exist
	auto iter = storage.recv_streams.find(p.stream_id());
	if (iter == storage.recv_streams.end()) {
		spdlog::debug("New stream: {}", p.stream_id());
		iter = storage.recv_streams.insert({
			p.stream_id(),
			std::move(RecvStream<NodeType>(p.stream_id(), node))
		}).first;
	}

	auto &stream = iter->second;

	// Short circuit once stream has been received fully.
	if(stream.state == RecvStream<NodeType>::State::AllRecv || stream.state == RecvStream<NodeType>::State::Read) return;

	if(p.is_fin_set() && stream.state == RecvStream<NodeType>::State::Recv) {
		stream.size = p.offset() + p.length();
		stream.state = RecvStream<NodeType>::State::SizeKnown;
	}

	auto stream_id = p.stream_id();
	auto offset = p.offset();
	auto length = p.length();
	auto packet_number = p.packet_number();

	// Cover header
	p.cover(22);

	StreamProtocol<NodeType>::send_ACK(node, addr, stream_id, packet_number);

	// Short circuit on no new data
	if(offset + length <= stream.read_offset) {
		return;
	}

	// Check if data can be read immediately
	if(offset <= stream.read_offset) {
		// Cover bytes which have already been read
		p.cover(stream.read_offset - offset);
		node.did_receive_bytes(std::move(p), stream.stream_id, addr);

		stream.read_offset = offset + length;

		// Read any out of order data
		auto iter = stream.recv_packets.begin();
		while(iter != stream.recv_packets.end()) {
			auto &packet = iter->second.packet;

			// Short circuit if data can't be read immediately
			if(iter->second.offset > stream.read_offset) {
				break;
			}

			// Check new data
			if(iter->second.offset + iter->second.length > stream.read_offset) {
				// Cover bytes which have already been read
				packet.cover(stream.read_offset - iter->second.offset);
				node.did_receive_bytes(std::move(packet), stream.stream_id, addr);

				stream.read_offset = iter->second.offset + iter->second.length;
			}

			// Next iter
			iter = stream.recv_packets.erase(iter);
		}

		// Check all data read
		if(stream.check_read()) {
			stream.state = RecvStream<NodeType>::State::Read;
			spdlog::info("Read: {}", node.num_bytes);
		}
	} else {
		// Queue packet for later processing
		stream.recv_packets[offset] = std::move(RecvPacketInfo(std::time(NULL), offset, length, std::move(p)));

		// Check all data received
		if (stream.check_finish()) {
			stream.state = RecvStream<NodeType>::State::AllRecv;
		}
	}
}

template<typename NodeType>
void StreamProtocol<NodeType>::send_ACK(NodeType &node, const net::SocketAddress &addr, uint16_t stream_id, uint64_t packet_number) {
	char *pdata = new char[12] {0, 2};

	uint16_t n_stream_id = htons(stream_id);
	std::memcpy(pdata+2, &n_stream_id, 2);

	uint64_t n_packet_number = htonll(packet_number);
	std::memcpy(pdata+4, &n_packet_number, 8);

	net::Packet p(pdata, 12);
	node.send(std::move(p), addr);
}

template<typename NodeType>
void StreamProtocol<NodeType>::did_receive_ACK(NodeType &node, const net::SocketAddress &addr, StreamPacket &&p) {
	spdlog::debug("ACK <<< {}", addr.to_string());

	auto &storage = node.stream_storage[addr];

	auto iter = storage.send_streams.find(p.stream_id());
	if (iter == storage.send_streams.end()) {
		spdlog::debug("Not found: {}", p.stream_id());
		return;
	}
	auto &stream = iter->second;

	// Short circuit if packet not found in sent.
	// Usually due to repeated ACKs. Malicious actors could use
	// spoofed ACKs to DDoS.
	if(stream.sent_packets.find(p.packet_number()) == stream.sent_packets.end())
		return;

	stream.bytes_in_flight -= stream.sent_packets[p.packet_number()].length;
	stream.sent_packets.erase(p.packet_number());

	// TODO: Check DataItem finish and discard

	// Check stream finish
	if (stream.sent_packets.size() == 0 && stream.state == SendStream::State::Sent) {
		stream.state = SendStream::State::Acked;

		uv_timer_stop(&stream.timer);
		delete (std::pair<net::SocketAddress, NodeType &> *)stream.timer.data;

		// TODO: Call delegate to inform
		spdlog::info("Acked: {}", p.stream_id());

		return;
	}

	auto sent_iter = stream.sent_packets.cbegin();
	auto now = std::time(nullptr);
	while(sent_iter != stream.sent_packets.cend()) {
		// Condition for packet in flight to be considered lost
		// 1. more than 3 packets before
		// 2. more than 25ms before
		if (sent_iter->first + 3 < p.packet_number() || std::difftime(now, sent_iter->second.sent_time) > 0.025) {
			// Retry lost packets
			auto &sent_packet = sent_iter->second;

			StreamProtocol<NodeType>::send_data_packet(node, addr, stream, *(sent_packet.data_item), sent_packet.offset, sent_packet.length);

			sent_iter = stream.sent_packets.erase(sent_iter);
		} else {
			break;
		}
	}

	// New packets
	_send_data(node, addr, stream);

	uv_timer_again(&stream.timer);
}

// Impl

} // namespace stream
} // namespace marlin

#endif // MARLIN_STREAM_STREAMPROTOCOL_HPP
