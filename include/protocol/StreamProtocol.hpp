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

		// Abort if data queue is too big
		if(stream.queue_offset - stream.acked_offset > 20000000)
			return;

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
		if(conn.sent_packets.size() == 0 && conn.lost_packets.size() == 0 && conn.send_queue.size() == 0) {
			uv_timer_start(&conn.timer, &StreamProtocol<NodeType>::timer_cb, conn.timer_interval, 0);
		}

		conn.register_send_intent(stream);
		conn.process_pending_data();
	}

	static void did_receive_DATA(NodeType &node, const net::SocketAddress &addr, StreamPacket &&p);
	static void send_DATA(NodeType &node, const net::SocketAddress &addr, net::Packet &&p);

	static void did_receive_ACK(NodeType &node, const net::SocketAddress &addr, StreamPacket &&p);
	static void send_ACK(NodeType &node, const net::SocketAddress &addr, const AckRanges &ack_ranges);

	static void ack_timer_cb(uv_timer_t *handle) {
		auto &conn = *(Connection<NodeType, NodeType> *)handle->data;

		StreamProtocol<NodeType>::send_ACK(conn.transport, conn.addr, conn.ack_ranges);

		conn.ack_timer_active = false;
	}

	static void timer_cb(uv_timer_t *handle) {
		auto &conn = *(Connection<NodeType, NodeType> *)handle->data;

		if(conn.sent_packets.size() == 0 && conn.lost_packets.size() == 0 && conn.send_queue.size() == 0) {
			// Idle connection, stop timer
			uv_timer_stop(&conn.timer);
			conn.timer_interval = DEFAULT_TLP_INTERVAL;
			return;
		}

		auto sent_iter = conn.sent_packets.cbegin();

		// Retry lost packets
		// No condition necessary, all are considered lost if tail probe fails
		while(sent_iter != conn.sent_packets.cend()) {
			conn.bytes_in_flight -= sent_iter->second.length;
			sent_iter->second.stream->bytes_in_flight -= sent_iter->second.length;
			conn.lost_packets[sent_iter->first] = sent_iter->second;

			sent_iter++;
		}

		if(sent_iter == conn.sent_packets.cbegin()) {
			// No lost packets, ignore
		} else {
			// Lost packets, congestion event
			auto last_iter = std::prev(sent_iter);
			auto &sent_packet = last_iter->second;
			if(sent_packet.sent_time > conn.congestion_start) {
				// New congestion event
				SPDLOG_ERROR("Timer congestion event: {}", conn.congestion_window);
				conn.congestion_start = uv_now(uv_default_loop());

				if(conn.congestion_window < conn.w_max) {
					// Fast convergence
					conn.w_max = conn.congestion_window;
					conn.congestion_window *= 0.6;
				} else {
					conn.w_max = conn.congestion_window;
					conn.congestion_window *= 0.75;
				}

				if(conn.congestion_window < 10000) {
					conn.congestion_window = 10000;
				}

				conn.ssthresh = conn.congestion_window;

				conn.k = std::cbrt(conn.w_max / 16)*1000;
			}

			// Pop lost packets from sent
			conn.sent_packets.erase(conn.sent_packets.cbegin(), sent_iter);
		}

		// New packets
		conn.process_pending_data();

		// Next timer interval
		// TODO: Abort on retrying too many times
		if(conn.timer_interval < 25000)
			conn.timer_interval *= 2;
		uv_timer_start(&conn.timer, &StreamProtocol<NodeType>::timer_cb, conn.timer_interval, 0);
	}
};

template<typename NodeType>
void StreamProtocol<NodeType>::send_DATA(NodeType &node, const net::SocketAddress &addr, net::Packet &&p) {
	node.send(std::move(p), addr);
}

template<typename NodeType>
void StreamProtocol<NodeType>::did_receive_DATA(NodeType &node, const net::SocketAddress &addr, StreamPacket &&p) {
	SPDLOG_TRACE("DATA <<< {}: {}, {}", addr.to_string(), p.offset(), p.length());

	auto offset = p.offset();
	auto length = p.length();
	auto packet_number = p.packet_number();

	// Get or create connection and stream
	auto &conn = node.stream_storage.try_emplace(
		addr,
		addr,
		node
	).first->second;

	if(conn.next_stream_id == 0) {
		conn.next_stream_id = 1;
	}

	auto &stream = conn.get_or_create_recv_stream(p.stream_id(), node);

	// Short circuit once stream has been received fully.
	if(stream.state == RecvStream<NodeType>::State::AllRecv ||
		stream.state == RecvStream<NodeType>::State::Read) {
		return;
	}

	// Set stream size if fin bit set
	if(p.is_fin_set() && stream.state == RecvStream<NodeType>::State::Recv) {
		stream.size = offset + length;
		stream.state = RecvStream<NodeType>::State::SizeKnown;
	}

	// Cover header
	p.cover(22);

	// Add to ack range
	conn.ack_ranges.add_packet_number(packet_number);

	// Start ack delay timer if not already active
	if(!conn.ack_timer_active) {
		conn.ack_timer_active = true;
		uv_timer_start(&conn.ack_timer, &ack_timer_cb, 25, 0);
	}

	// Short circuit on no new data
	if(offset + length <= stream.read_offset) {
		return;
	}

	// Check if data can be read immediately
	if(offset <= stream.read_offset) {
		// Cover bytes which have already been read
		p.cover(stream.read_offset - offset);

		// Read bytes and update offset
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

				// Read bytes and update offset
				node.did_receive_bytes(std::move(packet), stream.stream_id, addr);
				stream.read_offset = iter->second.offset + iter->second.length;
			}

			// Next iter
			iter = stream.recv_packets.erase(iter);
		}

		// Check all data read
		if(stream.check_read()) {
			stream.state = RecvStream<NodeType>::State::Read;
			conn.recv_streams.erase(stream.stream_id);
		}
	} else {
		// Queue packet for later processing
		stream.recv_packets[offset] = std::move(RecvPacketInfo(uv_now(uv_default_loop()), offset, length, std::move(p)));

		// Check all data received
		if (stream.check_finish()) {
			stream.state = RecvStream<NodeType>::State::AllRecv;
		}
	}
}

template<typename NodeType>
void StreamProtocol<NodeType>::send_ACK(NodeType &node, const net::SocketAddress &addr, const AckRanges &ack_ranges) {
	char *pdata = new char[500] {0, 2};

	int size = ack_ranges.ranges.size() > 61 ? 61 : ack_ranges.ranges.size();

	uint16_t n_size = htons(size);
	std::memcpy(pdata+2, &n_size, 2);

	uint64_t n_largest = htonll(ack_ranges.largest);
	std::memcpy(pdata+4, &n_largest, 8);

	int i = 12;
	auto iter = ack_ranges.ranges.begin();
	// Upto 30 gaps
	for(
		;
		i < 500 && iter != ack_ranges.ranges.end();
		i += 8, iter++
	) {
		uint64_t n_value = htonll(*iter);
		std::memcpy(pdata+i, &n_value, 8);
	}

	net::Packet p(pdata, 12 + size * 8);
	node.send(std::move(p), addr);
}

template<typename NodeType>
void StreamProtocol<NodeType>::did_receive_ACK(NodeType &node, const net::SocketAddress &addr, StreamPacket &&p) {
	SPDLOG_TRACE("ACK <<< {}", addr.to_string());

	// Get connection, short circuit if connection doesn't exist
	auto citer = node.stream_storage.find(addr);
	if(citer == node.stream_storage.end()) {
		return;
	}
	auto &conn = citer->second;

	auto now = uv_now(uv_default_loop());

	// TODO: Better method names
	uint16_t size = p.stream_id();
	uint64_t largest = p.packet_number();

	// New largest acked packet
	if(largest > conn.largest_acked && conn.sent_packets.find(largest) != conn.sent_packets.end()) {
		auto &sent_packet = conn.sent_packets[largest];

		// Update largest packet details
		conn.largest_acked = largest;
		conn.largest_sent_time = sent_packet.sent_time;

		// Update RTT estimate
		if(conn.rtt < 0) {
			conn.rtt = now - sent_packet.sent_time;
		} else {
			conn.rtt = 0.875 * conn.rtt + 0.125 * (now - sent_packet.sent_time);
		}

		// SPDLOG_INFO("RTT: {}", conn.rtt);
	}

	// Cover till range list
	p.cover(12);

	uint64_t high = largest;
	bool gap = false;
	bool is_app_limited = (conn.bytes_in_flight < 0.8 * conn.congestion_window);

	for(
		uint16_t i = 0;
		i < size;
		i++, gap = !gap
	) {
		uint64_t range = p.extract_uint64(i*8);

		uint64_t low = high - range;

		// Short circuit on gap range
		if(gap) {
			high = low;
			continue;
		}

		// Get packets within range [low+1, high]
		auto low_iter = conn.sent_packets.lower_bound(low + 1);
		auto high_iter = conn.sent_packets.upper_bound(high);

		// Iterate acked packets
		for(
			;
			low_iter != high_iter;
			low_iter = conn.sent_packets.erase(low_iter)
		) {
			auto &sent_packet = low_iter->second;
			auto &stream = *sent_packet.stream;

			auto sent_offset = sent_packet.data_item->stream_offset + sent_packet.offset;

			if(stream.acked_offset < sent_offset) {
				// Out of order ack, store for later processing
				stream.outstanding_acks[sent_offset] = sent_packet.length;
			} else if(stream.acked_offset < sent_offset + sent_packet.length) {
				// In order ack
				stream.acked_offset = sent_offset + sent_packet.length;

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
			} else {
				// Already acked range, ignore
			}

			// Cleanup
			stream.bytes_in_flight -= sent_packet.length;
			conn.bytes_in_flight -= sent_packet.length;

			SPDLOG_TRACE("Times: {}, {}", sent_packet.sent_time, conn.congestion_start);

			// Congestion control
			// Check if not in congestion recovery and not application limited
			if(sent_packet.sent_time > conn.congestion_start && !is_app_limited) {
				if(conn.congestion_window < conn.ssthresh) {
					// Slow start, exponential increase
					conn.congestion_window += sent_packet.length;
				} else {
					// Congestion avoidance, CUBIC
					// auto k = conn.k;
					// auto t = now - conn.congestion_start;
					// conn.congestion_window = conn.w_max + 4 * std::pow(0.001 * (t - k), 3);

					// if(conn.congestion_window < 10000) {
					// 	conn.congestion_window = 10000;
					// }

					// Congestion avoidance, NEW RENO
					conn.congestion_window += 1200 * sent_packet.length / conn.congestion_window;
				}
			}

			// Check stream finish
			if (stream.state == SendStream::State::Sent &&
				stream.acked_offset == stream.queue_offset) {
				stream.state = SendStream::State::Acked;

				// TODO: Call delegate to inform
				SPDLOG_DEBUG("Acked: {}", stream.stream_id);

				// Remove stream
				conn.send_streams.erase(stream.stream_id);

				return;
			}
		}

		high = low;
	}

	auto sent_iter = conn.sent_packets.cbegin();

	// Determine lost packets
	while(sent_iter != conn.sent_packets.cend()) {
		// Condition for packet in flight to be considered lost
		// 1. more than 20 packets before largest acked - disabled for now
		// 2. more than 25ms before before largest acked
		if (/*sent_iter->first + 20 < conn.largest_acked ||*/
			conn.largest_sent_time > sent_iter->second.sent_time + 25) {

			conn.bytes_in_flight -= sent_iter->second.length;
			sent_iter->second.stream->bytes_in_flight -= sent_iter->second.length;
			conn.lost_packets[sent_iter->first] = sent_iter->second;

			sent_iter++;
		} else {
			break;
		}
	}

	if(sent_iter == conn.sent_packets.cbegin()) {
		// No lost packets, ignore
	} else {
		// Lost packets, congestion event
		auto last_iter = std::prev(sent_iter);
		auto &sent_packet = last_iter->second;

		if(sent_packet.sent_time > conn.congestion_start) {
			// New congestion event
			SPDLOG_ERROR("Congestion event: {}", conn.congestion_window);
			conn.congestion_start = now;

			if(conn.congestion_window < conn.w_max) {
				// Fast convergence
				conn.w_max = conn.congestion_window;
				conn.congestion_window *= 0.6;
			} else {
				conn.w_max = conn.congestion_window;
				conn.congestion_window *= 0.75;
			}

			if(conn.congestion_window < 10000) {
				conn.congestion_window = 10000;
			}
			conn.ssthresh = conn.congestion_window;
			conn.k = std::cbrt(conn.w_max / 16)*1000;
		}

		// Pop lost packets from sent
		conn.sent_packets.erase(conn.sent_packets.cbegin(), sent_iter);
	}

	// New packets
	conn.process_pending_data();

	conn.timer_interval = DEFAULT_TLP_INTERVAL;
	uv_timer_start(&conn.timer, &StreamProtocol<NodeType>::timer_cb, conn.timer_interval, 0);
}

} // namespace stream
} // namespace marlin

#endif // MARLIN_STREAM_STREAMPROTOCOL_HPP
