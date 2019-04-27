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
using StreamStorage = std::map<net::SocketAddress, Connection<NodeType, NodeType>>;

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
			default: SPDLOG_TRACE("UNKNOWN <<< {}", addr.to_string());
			break;
		}
	}

	static void did_send_packet(
		NodeType &,
		net::Packet &&p,
		const net::SocketAddress &addr __attribute__((unused))
	) {
		auto pp = reinterpret_cast<StreamPacket *>(&p);
		switch(pp->message()) {
			// DATA
			case 0: SPDLOG_TRACE("DATA >>> {}", addr.to_string());
			break;
			// DATA + FIN
			case 1: SPDLOG_TRACE("DATA + FIN >>> {}", addr.to_string());
			break;
			// ACK
			case 2: SPDLOG_TRACE("ACK >>> {}", addr.to_string());
			break;
			// UNKNOWN
			default: SPDLOG_TRACE("UNKNOWN >>> {}", addr.to_string());
			break;
		}
	}

	// New stream
	static void send_data(
		NodeType &node,
		std::unique_ptr<char[]> &&data,
		uint64_t size,
		const net::SocketAddress &addr
	) {
		auto &conn = node.stream_storage.try_emplace(
			addr,
			addr,
			node
		).first->second;

		send_data(node, conn.next_stream_id, std::move(data), size, addr);

		conn.next_stream_id += 2;
	}

	// Explicit stream id. Stream created if not found.
	static void send_data(
		NodeType &node,
		uint16_t stream_id,
		std::unique_ptr<char[]> &&data,
		uint64_t size,
		const net::SocketAddress &addr
	) {
		auto &conn = node.stream_storage.try_emplace(
			addr,
			addr,
			node
		).first->second;

		auto &stream = conn.get_or_create_send_stream(stream_id);

		if(stream.state == SendStream::State::Ready) {
			stream.state = SendStream::State::Send;
		}

		// Add data to send queue
		stream.data_queue.emplace_back(
			std::move(data),
			size,
			stream.queue_offset
		);

		stream.queue_offset += size;

		// Handle idle stream
		if(stream.next_item_iterator == stream.data_queue.end()) {
			stream.next_item_iterator = std::prev(stream.data_queue.end());
		}

		// Handle idle connection
		if(conn.sent_packets.size() == 0 && conn.send_queue.size() == 0) {
			conn.timer_interval = DEFAULT_TLP_INTERVAL;
			uv_timer_start(&conn.timer, &StreamProtocol<NodeType>::timer_cb, conn.timer_interval, 0);
		}

		conn.register_send_intent(stream);
		conn.process_pending_data();
	}

	static void did_receive_DATA(NodeType &node, const net::SocketAddress &addr, StreamPacket &&p);
	static void send_DATA(NodeType &node, const net::SocketAddress &addr, net::Packet &&p);

	static void did_receive_ACK(NodeType &node, const net::SocketAddress &addr, StreamPacket &&p);
	static void send_ACK(NodeType &node, const net::SocketAddress &addr, uint16_t stream_id, uint64_t packet_number);

	static void timer_cb(uv_timer_t *handle) {
		auto &conn = *(Connection<NodeType, NodeType> *)handle->data;

		if(conn.sent_packets.size() == 0 && conn.send_queue.size() == 0) {
			// Idle connection, stop timer
			uv_timer_stop(&conn.timer);
			return;
		}

		auto sent_iter = conn.sent_packets.cbegin();
		auto num = conn.sent_packets.size();

		// Retry lost packets
		// No condition necessary, all are considered lost if tail probe fails
		for(size_t i = 0; i < num; i++) {
			// SPDLOG_DEBUG("{}, {}, {}", sent_iter->first, stream.sent_packets.size(), 0);

			auto &sent_packet = sent_iter->second;

			conn._send_data_packet(
				conn.addr,
				*(sent_packet.stream),
				*(sent_packet.data_item),
				sent_packet.offset,
				sent_packet.length
			);

			sent_iter = conn.sent_packets.erase(sent_iter);
		}

		// Next timer interval
		// TODO: Abort on retrying too many times
		if(conn.timer_interval < 25000)
			conn.timer_interval *= 2;
		uv_timer_start(&conn.timer, &StreamProtocol<NodeType>::timer_cb, conn.timer_interval, 0);

		SPDLOG_TRACE("Timer End");
	}
};

template<typename NodeType>
void StreamProtocol<NodeType>::send_DATA(NodeType &node, const net::SocketAddress &addr, net::Packet &&p) {
	node.send(std::move(p), addr);
}

template<typename NodeType>
void StreamProtocol<NodeType>::did_receive_DATA(NodeType &node, const net::SocketAddress &addr, StreamPacket &&p) {
	SPDLOG_TRACE("DATA <<< {}: {}, {}", addr.to_string(), p.offset(), p.length());

	auto &storage = node.stream_storage.try_emplace(
		addr,
		addr,
		node
	).first->second;

	if(storage.next_stream_id == 0) {
		storage.next_stream_id = 1;
	}

	auto &stream = storage.get_or_create_recv_stream(p.stream_id(), node);

	// Short circuit once stream has been received fully.
	if(stream.state == RecvStream<NodeType>::State::AllRecv ||
		stream.state == RecvStream<NodeType>::State::Read) {
		return;
	}

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
			storage.recv_streams.erase(stream.stream_id);
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
	SPDLOG_TRACE("ACK <<< {}", addr.to_string());

	// Get stream
	// Short circuit if connection or stream doesn't exist
	auto citer = node.stream_storage.find(addr);
	if(citer == node.stream_storage.end()) {
		return;
	}
	auto &conn = citer->second;

	auto siter = conn.send_streams.find(p.stream_id());
	if(siter == conn.send_streams.end()) {
		return;
	}
	auto &stream = siter->second;

	// Short circuit if packet not found in sent.
	// Usually due to repeated ACKs.
	auto piter = conn.sent_packets.find(p.packet_number());
	if(piter == conn.sent_packets.end())
		return;
	auto &sent_packet = piter->second;

	if(stream.acked_offset < sent_packet.offset) {
		// Out of order ack, store for later processing
		stream.outstanding_acks[sent_packet.offset] = sent_packet.length;
	} else {
		// In order ack
		stream.acked_offset = sent_packet.offset + sent_packet.length;

		// Process outstanding acks if possible
		for(
			auto iter = stream.outstanding_acks.begin();
			iter != stream.outstanding_acks.end();
			iter = stream.outstanding_acks.erase(iter)
		) {
			if(iter->first > stream.acked_offset) {
				// Out of order, abort
				break;
			}

			uint64_t new_acked_offset = iter->first + iter->second;
			if(stream.acked_offset < new_acked_offset) {
				stream.acked_offset = new_acked_offset;
			}
		}

		// Cleanup acked data items
		for(
			auto iter = stream.data_queue.begin();
			iter != stream.data_queue.end();
			iter = stream.data_queue.erase(iter)
		) {
			if(stream.acked_offset < iter->stream_offset + iter->size) {
				// Still not fully acked, skip erase and abort
				break;
			}

			// TODO: Call delegate
			SPDLOG_TRACE("Erase");
		}
	}

	// Cleanup
	stream.bytes_in_flight -= sent_packet.length;
	conn.bytes_in_flight -= sent_packet.length;
	conn.sent_packets.erase(p.packet_number());

	// Check stream finish
	if (stream.state == SendStream::State::Sent &&
		stream.acked_offset == stream.queue_offset) {
		stream.state = SendStream::State::Acked;

		// TODO: Call delegate to inform
		SPDLOG_DEBUG("Acked: {}", p.stream_id());

		// Remove stream
		conn.send_streams.erase(stream.stream_id);

		return;
	}

	auto sent_iter = conn.sent_packets.cbegin();
	auto now = std::time(nullptr);
	while(sent_iter != conn.sent_packets.cend()) {
		// Condition for packet in flight to be considered lost
		// 1. more than 3 packets before
		// 2. more than 25ms before
		if (sent_iter->first + 3 < p.packet_number() ||
			std::difftime(now, sent_iter->second.sent_time) > 0.025) {
			// Retry lost packets
			auto &sent_packet = sent_iter->second;

			conn._send_data_packet(
				addr,
				stream,
				*(sent_packet.data_item),
				sent_packet.offset,
				sent_packet.length
			);

			sent_iter = conn.sent_packets.erase(sent_iter);
		} else {
			break;
		}
	}

	// New packets
	conn.process_pending_data();

	conn.timer_interval = DEFAULT_TLP_INTERVAL;
	uv_timer_start(&conn.timer, &StreamProtocol<NodeType>::timer_cb, conn.timer_interval, 0);
}

} // namespace stream
} // namespace marlin

#endif // MARLIN_STREAM_STREAMPROTOCOL_HPP
