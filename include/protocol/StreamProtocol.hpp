#ifndef STREAM_STREAM_PROTOCOL_HPP
#define STREAM_STREAM_PROTOCOL_HPP

#include <marlin/fiber/Packet.hpp>
#include <marlin/fiber/SocketAddress.hpp>
#include "StreamPacket.hpp"
#include "SendStream.hpp"
#include "RecvStream.hpp"

#include <cstring>
#include <algorithm>

#include <spdlog/spdlog.h>

#include <iostream>

namespace stream {

template<typename NodeType>
struct PerPeerStreamStorage {
	std::map<uint8_t, SendStream> send_streams;
	std::map<uint8_t, RecvStream<NodeType>> recv_streams;

	// TODO: Better resolve id collisions
	uint8_t next_stream_id = 0;
};

template<typename NodeType>
using StreamStorage = std::map<fiber::SocketAddress, PerPeerStreamStorage<NodeType>>;

template<typename NodeType>
class StreamProtocol {
public:
	// static void setup(NodeType &node);

	static void did_receive_packet(
		NodeType &node,
		fiber::Packet &&p,
		const fiber::SocketAddress &addr
	) {
		auto pp = reinterpret_cast<StreamPacket *>(&p);
		switch(pp->message()) {
			// DATA
			case 0: did_receive_DATA(node, addr, std::move(*pp));
			break;
			// FIN
			case 1: did_receive_FIN(node, addr, std::move(*pp));
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
		fiber::Packet &&p,
		const fiber::SocketAddress &addr
	) {
		auto pp = reinterpret_cast<StreamPacket *>(&p);
		switch(pp->message()) {
			// DATA
			case 0: spdlog::info("DATA >>> {}", addr.to_string());;
			break;
			// FIN
			case 1: spdlog::info("FIN >>> {}", addr.to_string());;
			break;
			// ACK
			case 2: spdlog::info("ACK >>> {}", addr.to_string());;
			break;
			// UNKNOWN
			default: spdlog::info("UNKNOWN >>> {}", addr.to_string());
			break;
		}
	}

	static void send_data(NodeType &node, std::unique_ptr<char[]> &&data, size_t size, const fiber::SocketAddress &addr) {
		auto &storage = node.stream_storage[addr];

		SendStream &stream = storage.send_streams.insert({
			storage.next_stream_id,
			std::move(SendStream(storage.next_stream_id, std::move(data), size))
		}).first->second;
		storage.next_stream_id++;
		stream.state = SendStream::State::Send;

		for(uint64_t i = 0; i < stream.size; i+=1000) {
			char *pdata = new char[1100] {4, 0, 0, (char)stream.stream_id};

			uint64_t n_packet_number = htonll(stream.last_sent_packet+1);
			std::memcpy(pdata+4, &n_packet_number, 8);

			uint64_t n_offset = htonll(i);
			std::memcpy(pdata+12, &n_offset, 8);

			uint16_t dsize = std::min<size_t>(stream.size - i, 1000);
			uint16_t n_length = htons(dsize);
			std::memcpy(pdata+20, &n_length, 2);

			std::memcpy(pdata+22, stream.data.get()+i, dsize);

			stream.sent_packets[stream.last_sent_packet+1] = SentPacketInfo(std::time(NULL), i, dsize);
			stream.last_sent_packet++;

			fiber::Packet p(pdata, dsize+22);
			StreamProtocol<NodeType>::send_DATA(node, addr, std::move(p));
		}

		stream.state = SendStream::State::Sent;
		StreamProtocol<NodeType>::send_FIN(node, addr, stream);
	}

	static void did_receive_DATA(NodeType &node, const fiber::SocketAddress &addr, StreamPacket &&p);
	static void send_DATA(NodeType &node, const fiber::SocketAddress &addr, fiber::Packet &&p);

	static void did_receive_FIN(NodeType &node, const fiber::SocketAddress &addr, StreamPacket &&p);
	static void send_FIN(NodeType &node, const fiber::SocketAddress &addr, SendStream &stream);

	static void did_receive_ACK(NodeType &node, const fiber::SocketAddress &addr, StreamPacket &&p);
	static void send_ACK(NodeType &node, const fiber::SocketAddress &addr, uint8_t stream_id, uint64_t packet_number);
};

template<typename NodeType>
void StreamProtocol<NodeType>::send_DATA(NodeType &node, const fiber::SocketAddress &addr, fiber::Packet &&p) {
	node.send(std::move(p), addr);
}

template<typename NodeType>
void StreamProtocol<NodeType>::did_receive_DATA(NodeType &node, const fiber::SocketAddress &addr, StreamPacket &&p) {
	spdlog::info("DATA <<< {}", addr.to_string());

	auto &storage = node.stream_storage[addr];

	// Create stream if it does not exist
	auto iter = storage.recv_streams.find(p.stream_id());
	if (iter == storage.recv_streams.end()) {
		spdlog::debug("New stream: {}", (int)p.stream_id());
		iter = storage.recv_streams.insert({
			p.stream_id(),
			std::move(RecvStream<NodeType>(p.stream_id(), node))
		}).first;
	}

	auto &stream = iter->second;

	auto stream_id = p.stream_id();
	auto offset = p.offset();
	auto length = p.length();
	auto packet_number = p.packet_number();

	stream.recv_packets[offset] = std::move(RecvPacketInfo(std::time(NULL), offset, length, std::move(p._data)));

	StreamProtocol<NodeType>::send_ACK(node, addr, stream_id, packet_number);

	if (stream.check_finish()) {
		std::unique_ptr<char[]> message(new char[stream.size]);
		for (auto iter = stream.recv_packets.begin(); iter != stream.recv_packets.end(); iter++) {
			if (iter->second.length == 0) continue;
			memcpy(message.get()+iter->second.offset, iter->second.data.get(), iter->second.length);
		}

		stream.delegate.did_receive_message(std::move(message), stream.size);
	}
}

template<typename NodeType>
void StreamProtocol<NodeType>::send_FIN(NodeType &node, const fiber::SocketAddress &addr, SendStream &stream) {
	char *pdata = new char[20] {4, 0, 1, (char)stream.stream_id};

	uint64_t n_packet_number = htonll(stream.last_sent_packet+1);
	std::memcpy(pdata+4, &n_packet_number, 8);

	uint64_t n_offset = htonll(stream.size);
	std::memcpy(pdata+12, &n_offset, 8);

	stream.sent_packets[stream.last_sent_packet+1] = SentPacketInfo(std::time(NULL), stream.size, 0);
	stream.last_sent_packet++;
	fiber::Packet p(pdata, 20);
	node.send(std::move(p), addr);
}

template<typename NodeType>
void StreamProtocol<NodeType>::did_receive_FIN(NodeType &node, const fiber::SocketAddress &addr, StreamPacket &&p) {
	spdlog::info("FIN <<< {}", addr.to_string());

	auto &storage = node.stream_storage[addr];

	auto iter = storage.recv_streams.find(p.stream_id());
	if (iter == storage.recv_streams.end()) {
		iter = storage.recv_streams.insert({
			p.stream_id(),
			std::move(RecvStream<NodeType>(p.stream_id(), node))
		}).first;
	}

	auto &stream = iter->second;

	stream.recv_packets[p.offset()] = RecvPacketInfo(std::time(NULL), p.offset(), 0, nullptr);
	stream.size = p.offset();
	if (stream.state == RecvStream<NodeType>::State::Recv) {
		stream.state = RecvStream<NodeType>::State::SizeKnown;
	}

	StreamProtocol<NodeType>::send_ACK(node, addr, p.stream_id(), p.packet_number());

	if (stream.check_finish()) {
		std::unique_ptr<char[]> message(new char[stream.size]);
		for (auto iter = stream.recv_packets.begin(); iter != stream.recv_packets.end(); iter++) {
			if (iter->second.length == 0) continue;
			memcpy(message.get()+iter->second.offset, iter->second.data.get(), iter->second.length);
		}

		stream.delegate.did_receive_message(std::move(message), stream.size);
	}
}

template<typename NodeType>
void StreamProtocol<NodeType>::send_ACK(NodeType &node, const fiber::SocketAddress &addr, uint8_t stream_id, uint64_t packet_number) {
	char *pdata = new char[12] {4, 0, 2, (char)stream_id};

	uint64_t n_packet_number = htonll(packet_number);
	std::memcpy(pdata+4, &n_packet_number, 8);

	fiber::Packet p(pdata, 12);
	node.send(std::move(p), addr);
}

template<typename NodeType>
void StreamProtocol<NodeType>::did_receive_ACK(NodeType &node, const fiber::SocketAddress &addr, StreamPacket &&p) {
	spdlog::info("ACK <<< {}", addr.to_string());

	auto &storage = node.stream_storage[addr];

	auto iter = storage.send_streams.find(p.stream_id());
	if (iter == storage.send_streams.end()) {
		spdlog::debug("Not found: {}", (int)p.stream_id());
		return;
	}
	auto &stream = iter->second;

	stream.sent_packets.erase(p.packet_number());

	if (stream.sent_packets.size() == 0 && stream.state == SendStream::State::Sent) {
		stream.state = SendStream::State::Acked;

		// TODO: Call delegate to inform
		spdlog::debug("Acked: {}", (int)p.stream_id());
	}
}

// Impl

} // namespace stream

#endif // STREAM_STREAM_PROTOCOL_HPP
