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
	static void setup(NodeType &node) {}

	static void did_receive_packet(
		NodeType &node,
		net::Packet &&p,
		const net::SocketAddress &addr
	) {
		auto pp = reinterpret_cast<StreamPacket *>(&p);
		switch(pp->message()) {
			// DATA
			case 0:
			case 1: did_receive_DATA(node, addr, std::move(*pp));
			break;
			// ACK
			case 2: did_receive_ACK(node, addr, std::move(*pp));
			break;
			// UNKNOWN
			default: spdlog::info("UNKNOWN <<< {}", addr.to_string());
			break;
		}
	}

	static void did_send_packet(
		NodeType &node,
		net::Packet &&p,
		const net::SocketAddress &addr
	) {
		auto pp = reinterpret_cast<StreamPacket *>(&p);
		switch(pp->message()) {
			// DATA
			case 0: spdlog::info("DATA >>> {}", addr.to_string());;
			break;
			// DATA + FIN
			case 1: spdlog::info("DATA + FIN >>> {}", addr.to_string());;
			break;
			// ACK
			case 2: spdlog::info("ACK >>> {}", addr.to_string());;
			break;
			// UNKNOWN
			default: spdlog::info("UNKNOWN >>> {}", addr.to_string());
			break;
		}
	}

	static void send_data(NodeType &node, std::unique_ptr<char[]> &&data, size_t size, const net::SocketAddress &addr) {
		auto &conn = node.stream_storage[addr];

		SendStream &stream = conn.send_streams.insert({
			conn.next_stream_id,
			std::move(SendStream(conn.next_stream_id, std::move(data), size))
		}).first->second;

		conn.next_stream_id += 2;
		stream.state = SendStream::State::Send;

		for(uint64_t i = 0; i < stream.size; i+=1000) {
			bool is_fin = (i + 1000 >= stream.size);

			char *pdata = new char[1100] {0, is_fin ? 1 : 0};

			uint16_t n_stream_id = htons(stream.stream_id);
			std::memcpy(pdata+2, &n_stream_id, 2);

			uint64_t n_packet_number = htonll(stream.last_sent_packet+1);
			std::memcpy(pdata+4, &n_packet_number, 8);

			uint64_t n_offset = htonll(i);
			std::memcpy(pdata+12, &n_offset, 8);

			uint16_t dsize = is_fin ? stream.size - i : 1000;
			uint16_t n_length = htons(dsize);
			std::memcpy(pdata+20, &n_length, 2);

			std::memcpy(pdata+22, stream.data.get()+i, dsize);

			stream.sent_packets[stream.last_sent_packet+1] = SentPacketInfo(std::time(NULL), i, dsize);
			stream.last_sent_packet++;

			net::Packet p(pdata, dsize+22);
			StreamProtocol<NodeType>::send_DATA(node, addr, std::move(p));
		}

		stream.state = SendStream::State::Sent;

		stream.timer.data = new std::tuple<net::SocketAddress, NodeType &, SendStream &>(addr, node, stream);
		uv_timer_start(&stream.timer, &StreamProtocol<NodeType>::timer_cb, 25, 25);
	}

	static void did_receive_DATA(NodeType &node, const net::SocketAddress &addr, StreamPacket &&p);
	static void send_DATA(NodeType &node, const net::SocketAddress &addr, net::Packet &&p);

	static void did_receive_ACK(NodeType &node, const net::SocketAddress &addr, StreamPacket &&p);
	static void send_ACK(NodeType &node, const net::SocketAddress &addr, uint16_t stream_id, uint64_t packet_number);

	static void timer_cb(uv_timer_t *handle) {
		spdlog::debug("Timer Begin");

		auto &tuple = *(std::tuple<net::SocketAddress, NodeType &, SendStream &> *)handle->data;
		auto &node = std::get<1>(tuple);
		auto addr = std::get<0>(tuple);
		auto &stream = std::get<2>(tuple);

		auto sent_iter = stream.sent_packets.cbegin();
		auto num = stream.sent_packets.size();
		auto now = std::time(nullptr);

		for(int i = 0; i < num; i++) {
			spdlog::debug("{}, {}, {}", sent_iter->first, stream.sent_packets.size(), 0);

			auto &sent_packet = sent_iter->second;

			bool is_fin = (sent_packet.offset + sent_packet.length >= stream.size);

			char *pdata = new char[1100] {0, is_fin ? 1 : 0};

			uint16_t n_stream_id = htons(stream.stream_id);
			std::memcpy(pdata+2, &n_stream_id, 2);

			uint64_t n_packet_number = htonll(stream.last_sent_packet+1);
			std::memcpy(pdata+4, &n_packet_number, 8);

			uint64_t n_offset = htonll(sent_packet.offset);
			std::memcpy(pdata+12, &n_offset, 8);

			uint16_t n_length = htons(sent_packet.length);
			std::memcpy(pdata+20, &n_length, 2);

			std::memcpy(pdata+22, stream.data.get()+sent_packet.offset, sent_packet.length);

			stream.sent_packets[stream.last_sent_packet+1] = SentPacketInfo(now, sent_packet.offset, sent_packet.length);
			stream.last_sent_packet++;

			net::Packet p(pdata, sent_packet.length+22);
			StreamProtocol<NodeType>::send_DATA(node, addr, std::move(p));

			sent_iter = stream.sent_packets.erase(sent_iter);
		}

		spdlog::debug("Timer End");
	}
};

template<typename NodeType>
void StreamProtocol<NodeType>::send_DATA(NodeType &node, const net::SocketAddress &addr, net::Packet &&p) {
	node.send(std::move(p), addr);
}

template<typename NodeType>
void StreamProtocol<NodeType>::did_receive_DATA(NodeType &node, const net::SocketAddress &addr, StreamPacket &&p) {
	spdlog::info("DATA <<< {}", addr.to_string());

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

	stream.recv_packets[offset] = std::move(RecvPacketInfo(std::time(NULL), offset, length, std::move(p)));

	StreamProtocol<NodeType>::send_ACK(node, addr, stream_id, packet_number);

	if (stream.check_finish()) {
		std::unique_ptr<char[]> message(new char[stream.size]);
		for (auto iter = stream.recv_packets.begin(); iter != stream.recv_packets.end(); iter++) {
			if (iter->second.length == 0) continue;
			memcpy(message.get()+iter->second.offset, iter->second.packet.data(), iter->second.length);
		}

		stream.delegate.did_receive_message(std::move(message), stream.size);
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
	spdlog::info("ACK <<< {}", addr.to_string());

	auto &storage = node.stream_storage[addr];

	auto iter = storage.send_streams.find(p.stream_id());
	if (iter == storage.send_streams.end()) {
		spdlog::debug("Not found: {}", p.stream_id());
		return;
	}
	auto &stream = iter->second;

	stream.sent_packets.erase(p.packet_number());

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
		if (sent_iter->first + 3 < p.packet_number() || std::difftime(now, sent_iter->second.sent_time) > 0.025) {
			auto &sent_packet = sent_iter->second;

			bool is_fin = (sent_packet.offset + sent_packet.length >= stream.size);

			char *pdata = new char[1100] {0, is_fin ? 1 : 0};

			uint16_t n_stream_id = htons(stream.stream_id);
			std::memcpy(pdata+2, &n_stream_id, 2);

			uint64_t n_packet_number = htonll(stream.last_sent_packet+1);
			std::memcpy(pdata+4, &n_packet_number, 8);

			uint64_t n_offset = htonll(sent_packet.offset);
			std::memcpy(pdata+12, &n_offset, 8);

			uint16_t n_length = htons(sent_packet.length);
			std::memcpy(pdata+20, &n_length, 2);

			std::memcpy(pdata+22, stream.data.get()+sent_packet.offset, sent_packet.length);

			stream.sent_packets[stream.last_sent_packet+1] = SentPacketInfo(now, sent_packet.offset, sent_packet.length);
			stream.last_sent_packet++;

			net::Packet p(pdata, sent_packet.length+22);
			StreamProtocol<NodeType>::send_DATA(node, addr, std::move(p));

			sent_iter = stream.sent_packets.erase(sent_iter);
		} else {
			break;
		}
	}

	uv_timer_again(&stream.timer);
}

// Impl

} // namespace stream
} // namespace marlin

#endif // MARLIN_STREAM_STREAMPROTOCOL_HPP
