#ifndef MARLIN_STREAM_STREAMTRANSPORT_HPP
#define MARLIN_STREAM_STREAMTRANSPORT_HPP

#include <spdlog/spdlog.h>
#include <unordered_set>
#include <unordered_map>
#include <random>

#include <marlin/net/SocketAddress.hpp>
#include <marlin/net/Buffer.hpp>
#include <marlin/net/core/TransportManager.hpp>

#include "protocol/SendStream.hpp"
#include "protocol/RecvStream.hpp"
#include "protocol/AckRanges.hpp"

namespace marlin {
namespace stream {

#define DEFAULT_TLP_INTERVAL 1000
#define DEFAULT_PACING_LIMIT 50000
#define DEFAULT_FRAGMENT_SIZE 1400

template<typename DelegateType, template<typename> class DatagramTransport>
class StreamTransport {
private:
	typedef DatagramTransport<StreamTransport<DelegateType, DatagramTransport>> BaseTransport;
	BaseTransport &transport;
	net::TransportManager<StreamTransport<DelegateType, DatagramTransport>> &transport_manager;

	void reset();

	// Connection
	enum struct ConnectionState {
		Listen,
		DialSent,
		DialRcvd,
		Established
	} conn_state = ConnectionState::Listen;
	uint32_t src_conn_id = 0;
	uint32_t dst_conn_id = 0;
	bool dialled = false;
	uv_timer_t state_timer;
	uint64_t state_timer_interval = 0;

	static void dial_timer_cb(uv_timer_t *handle);

	// Streams
	std::unordered_map<uint16_t, SendStream> send_streams;
	std::unordered_map<uint16_t, RecvStream> recv_streams;

	SendStream &get_or_create_send_stream(uint16_t const stream_id);
	RecvStream &get_or_create_recv_stream(uint16_t const stream_id);

	// Packets
	uint64_t last_sent_packet = -1;
	std::map<uint64_t, SentPacketInfo> sent_packets;
	std::map<uint64_t, SentPacketInfo> lost_packets;

	// RTT estimate
	double rtt = -1;

	// Congestion control
	uint64_t bytes_in_flight = 0;
	uint64_t k = 0;
	uint64_t w_max = 0;
	uint64_t congestion_window = 15000;
	uint64_t ssthresh = -1;
	uint64_t congestion_start = 0;
	uint64_t largest_acked = 0;
	uint64_t largest_sent_time = 0;

	// Send
	std::unordered_set<uint16_t> send_queue_ids;
	std::list<SendStream *> send_queue;

	bool register_send_intent(SendStream &stream);

	void send_data_packet(
		SendStream &stream,
		DataItem &data_item,
		uint64_t offset,
		uint16_t length
	);

	void send_pending_data();
	int send_lost_data(uint64_t initial_bytes_in_flight);
	int send_new_data(SendStream &stream, uint64_t initial_bytes_in_flight);

	// Pacing
	uv_timer_t pacing_timer;
	bool is_pacing_timer_active = false;

	static void pacing_timer_cb(uv_timer_t *handle);

	// TLP
	uv_timer_t tlp_timer;
	uint64_t tlp_interval = DEFAULT_TLP_INTERVAL;

	static void tlp_timer_cb(uv_timer_t *handle);

	// ACKs
	AckRanges ack_ranges;
	uv_timer_t ack_timer;
	bool ack_timer_active = false;

	static void ack_timer_cb(uv_timer_t *handle);

	// Protocol
	void send_DIAL();
	void did_recv_DIAL(net::Buffer &&packet);

	void send_DIALCONF();
	void did_recv_DIALCONF(net::Buffer &&packet);

	void send_CONF();
	void did_recv_CONF(net::Buffer &&packet);

	void send_RST(uint32_t src_conn_id, uint32_t dst_conn_id);
	void did_recv_RST(net::Buffer &&packet);

	void send_DATA(net::Buffer &&packet);
	void did_recv_DATA(net::Buffer &&packet);

	void send_ACK();
	void did_recv_ACK(net::Buffer &&packet);

public:
	// Delegate
	void did_dial(BaseTransport &transport);
	void did_recv_packet(BaseTransport &transport, net::Buffer &&packet);
	void did_send_packet(BaseTransport &transport, net::Buffer &&packet);

	net::SocketAddress src_addr;
	net::SocketAddress dst_addr;

	DelegateType *delegate;

	StreamTransport(
		net::SocketAddress const &src_addr,
		net::SocketAddress const &dst_addr,
		BaseTransport &transport,
		net::TransportManager<StreamTransport<DelegateType, DatagramTransport>> &transport_manager
	);

	void setup(DelegateType *delegate);
	int send(net::Buffer &&bytes);
	void close();

	bool is_active();
};


// Impl

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::reset() {
	// Reset transport
	conn_state = ConnectionState::Listen;
	src_conn_id = 0;
	dst_conn_id = 0;
	dialled = false;
	uv_timer_stop(&state_timer);
	state_timer_interval = 0;

	send_streams.clear();
	recv_streams.clear();

	last_sent_packet = -1;
	sent_packets.clear();
	lost_packets.clear();

	rtt = -1;

	bytes_in_flight = 0;
	k = 0;
	w_max = 0;
	congestion_window = 15000;
	ssthresh = -1;
	congestion_start = 0;
	largest_acked = 0;
	largest_sent_time = 0;

	send_queue_ids.clear();
	send_queue.clear();

	uv_timer_stop(&pacing_timer);
	is_pacing_timer_active = false;

	uv_timer_stop(&tlp_timer);
	tlp_interval = DEFAULT_TLP_INTERVAL;

	ack_ranges = AckRanges();
	uv_timer_stop(&ack_timer);
	ack_timer_active = false;
}

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::dial_timer_cb(
	uv_timer_t *handle
) {
	auto &transport = *(StreamTransport<DelegateType, DatagramTransport> *)handle->data;

	if(transport.state_timer_interval >= 64000) { // Abort on too many retries
		transport.state_timer_interval = 0;
		SPDLOG_ERROR("Stream protocol: Dial timeout: {}", transport.dst_addr.to_string());
		return;
	}

	transport.send_DIAL();
	transport.state_timer_interval *= 2;
	uv_timer_start(
		&transport.state_timer,
		dial_timer_cb,
		transport.state_timer_interval,
		0
	);
}

//---------------- Stream functions begin ----------------//

template<typename DelegateType, template<typename> class DatagramTransport>
SendStream &StreamTransport<DelegateType, DatagramTransport>::get_or_create_send_stream(
	uint16_t const stream_id
) {
	auto iter = send_streams.try_emplace(
		stream_id,
		stream_id
	).first;

	return iter->second;
}

template<typename DelegateType, template<typename> class DatagramTransport>
RecvStream &StreamTransport<DelegateType, DatagramTransport>::get_or_create_recv_stream(
	uint16_t const stream_id
) {
	auto iter = recv_streams.try_emplace(
		stream_id,
		stream_id
	).first;

	return iter->second;
}

//---------------- Stream functions end ----------------//


//---------------- Send functions end ----------------//

template<typename DelegateType, template<typename> class DatagramTransport>
bool StreamTransport<DelegateType, DatagramTransport>::register_send_intent(
	SendStream &stream
) {
	if(send_queue_ids.find(stream.stream_id) != send_queue_ids.end()) {
		return false;
	}

	send_queue.push_back(&stream);
	send_queue_ids.insert(stream.stream_id);

	return true;
}

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::send_data_packet(
	SendStream &stream,
	DataItem &data_item,
	uint64_t offset,
	uint16_t length
) {
	bool is_fin = (stream.done_queueing &&
		data_item.stream_offset + offset + length >= stream.queue_offset);

	net::Buffer packet(
		new char[length + 30] {0, static_cast<char>(is_fin)},
		length + 30
	);

	this->last_sent_packet++;

	packet.write_uint32_be(2, src_conn_id);
	packet.write_uint32_be(6, dst_conn_id);
	packet.write_uint16_be(10, stream.stream_id);
	packet.write_uint64_be(12, this->last_sent_packet);
	packet.write_uint64_be(20, data_item.stream_offset + offset);
	packet.write_uint16_be(28, length);

	std::memcpy(packet.data()+30, data_item.data.get()+offset, length);

	this->sent_packets.emplace(
		std::piecewise_construct,
		std::forward_as_tuple(this->last_sent_packet),
		std::forward_as_tuple(
			uv_now(uv_default_loop()),
			&stream,
			&data_item,
			offset,
			length
		)
	);

	send_DATA(std::move(packet));

	if(is_fin && stream.state != SendStream::State::Acked) {
		stream.state = SendStream::State::Sent;
	}
}

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::send_pending_data() {
	if(is_pacing_timer_active == false) {
		is_pacing_timer_active = true;
		uv_timer_start(&pacing_timer, &pacing_timer_cb, 0, 0);
	}
}

template<typename DelegateType, template<typename> class DatagramTransport>
int StreamTransport<DelegateType, DatagramTransport>::send_lost_data(
	uint64_t initial_bytes_in_flight
) {
	for(
		auto iter = lost_packets.begin();
		iter != lost_packets.end();
		iter = lost_packets.erase(iter)
	) {
		if(bytes_in_flight - initial_bytes_in_flight >= DEFAULT_PACING_LIMIT) {
			// Pacing limit hit, reschedule timer
			is_pacing_timer_active = true;
			uv_timer_start(&pacing_timer, &pacing_timer_cb, 1, 0);
			return -1;
		}

		auto &sent_packet = iter->second;
		if(bytes_in_flight > congestion_window - sent_packet.length)
			return -2;

		send_data_packet(
			*sent_packet.stream,
			*sent_packet.data_item,
			sent_packet.offset,
			sent_packet.length
		);

		sent_packet.stream->bytes_in_flight += sent_packet.length;
		bytes_in_flight += sent_packet.length;

		SPDLOG_DEBUG("Lost packet sent: {}, {}", sent_packet.offset, last_sent_packet);
	}

	return 0;
}

template<typename DelegateType, template<typename> class DatagramTransport>
int StreamTransport<DelegateType, DatagramTransport>::send_new_data(
	SendStream &stream,
	uint64_t initial_bytes_in_flight
) {
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

			if(this->bytes_in_flight > this->congestion_window - dsize)
				return -2;

			send_data_packet(stream, data_item, i, dsize);

			stream.bytes_in_flight += dsize;
			stream.sent_offset += dsize;
			this->bytes_in_flight += dsize;
			data_item.sent_offset += dsize;

			if(this->bytes_in_flight - initial_bytes_in_flight >= DEFAULT_PACING_LIMIT) {
				return -1;
			}
		}
	}

	return 0;
}

//---------------- Send functions end ----------------//


//---------------- Pacing functions begin ----------------//

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::pacing_timer_cb(uv_timer_t *handle) {
	auto &transport = *(StreamTransport<DelegateType, DatagramTransport> *)handle->data;
	transport.is_pacing_timer_active = false;

	auto initial_bytes_in_flight = transport.bytes_in_flight;

	auto res = transport.send_lost_data(initial_bytes_in_flight);
	if(res < 0) {
		return;
	}

	// New packets
	for(
		auto iter = transport.send_queue.begin();
		iter != transport.send_queue.end();
		// Empty
	) {
		auto &stream = **iter;

		int res = transport.send_new_data(stream, initial_bytes_in_flight);
		if(res == 0) { // Idle stream, move to next stream
			transport.send_queue_ids.erase(stream.stream_id);
			iter = transport.send_queue.erase(iter);
		} else if(res == -1) { // Pacing limit hit, reschedule timer
			// Pacing limit hit, reschedule timer
			transport.is_pacing_timer_active = true;
			uv_timer_start(handle, &pacing_timer_cb, 1, 0);
			return;
		} else { // Congestion window exhausted, break
			return;
		}
	}
}

//---------------- Pacing functions end ----------------//


//---------------- TLP functions begin ----------------//

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::tlp_timer_cb(uv_timer_t *handle) {
	auto &transport = *(StreamTransport<DelegateType, DatagramTransport> *)handle->data;

	if(transport.sent_packets.size() == 0 && transport.lost_packets.size() == 0 && transport.send_queue.size() == 0) {
		// Idle connection, stop timer
		uv_timer_stop(&transport.tlp_timer);
		transport.tlp_interval = DEFAULT_TLP_INTERVAL;
		return;
	}

	auto sent_iter = transport.sent_packets.cbegin();

	// Retry lost packets
	// No condition necessary, all are considered lost if tail probe fails
	while(sent_iter != transport.sent_packets.cend()) {
		transport.bytes_in_flight -= sent_iter->second.length;
		sent_iter->second.stream->bytes_in_flight -= sent_iter->second.length;
		transport.lost_packets[sent_iter->first] = sent_iter->second;

		sent_iter++;
	}

	if(sent_iter == transport.sent_packets.cbegin()) {
		// No lost packets, ignore
	} else {
		// Lost packets, congestion event
		auto last_iter = std::prev(sent_iter);
		auto &sent_packet = last_iter->second;
		if(sent_packet.sent_time > transport.congestion_start) {
			// New congestion event
			SPDLOG_ERROR("Stream protocol: Timer congestion event: {}", transport.congestion_window);
			transport.congestion_start = uv_now(uv_default_loop());

			if(transport.congestion_window < transport.w_max) {
				// Fast convergence
				transport.w_max = transport.congestion_window;
				transport.congestion_window *= 0.6;
			} else {
				transport.w_max = transport.congestion_window;
				transport.congestion_window *= 0.75;
			}

			if(transport.congestion_window < 10000) {
				transport.congestion_window = 10000;
			}

			transport.ssthresh = transport.congestion_window;

			transport.k = std::cbrt(transport.w_max / 16)*1000;
		}

		// Pop lost packets from sent
		transport.sent_packets.erase(transport.sent_packets.cbegin(), sent_iter);
	}

	// New packets
	transport.send_pending_data();

	// Next timer interval
	// TODO: Abort on retrying too many times
	if(transport.tlp_interval < 25000)
		transport.tlp_interval *= 2;
	uv_timer_start(&transport.tlp_timer, &tlp_timer_cb, transport.tlp_interval, 0);
}

//---------------- TLP functions end ----------------//


//---------------- ACK functions begin ----------------//

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::ack_timer_cb(uv_timer_t *handle) {
	auto &transport = *(StreamTransport<DelegateType, DatagramTransport> *)handle->data;

	transport.send_ACK();

	transport.ack_timer_active = false;
}

//---------------- ACK functions end ----------------//


//---------------- Protocol functions begin ----------------//

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::send_DIAL() {
	net::Buffer packet(new char[10] {0, 3}, 10);

	packet.write_uint32_be(2, src_conn_id);
	packet.write_uint32_be(6, dst_conn_id);

	transport.send(std::move(packet));
}

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::did_recv_DIAL(
	net::Buffer &&packet
) {
	if(conn_state == ConnectionState::Listen) {
		if(packet.read_uint32_be(6) != 0) { // Should have empty source
			SPDLOG_ERROR(
				"Stream protocol: DIAL: Should have empty src: {}",
				packet.read_uint32_be(6)
			);
			return;
		}

		dst_conn_id = packet.read_uint32_be(2);
		src_conn_id = (uint32_t)std::random_device()();

		send_DIALCONF();

		conn_state = ConnectionState::DialRcvd;
	} else if(conn_state == ConnectionState::DialSent) {
		if(packet.read_uint32_be(6) != 0) { // Should have empty source
			SPDLOG_ERROR(
				"Stream protocol: DIAL: Should have empty src: {}",
				packet.read_uint32_be(6)
			);
			return;
		}

		dst_conn_id = packet.read_uint32_be(2);

		uv_timer_stop(&state_timer);
		state_timer_interval = 0;

		send_DIALCONF();

		conn_state = ConnectionState::DialRcvd;
	} else if(conn_state == ConnectionState::DialRcvd) {
		// Send existing ids
		// Wait for dst to RST and try to establish again
		// if this DIAL is latest one
		send_DIALCONF();
	} else if(conn_state == ConnectionState::Established) {
		// Send existing ids
		// Wait for dst to RST and try to establish again
		// if this connection is stale
		send_DIALCONF();
	}
}

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::send_DIALCONF() {
	net::Buffer packet(new char[10] {0, 4}, 10);

	packet.write_uint32_be(2, src_conn_id);
	packet.write_uint32_be(6, dst_conn_id);

	transport.send(std::move(packet));
}

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::did_recv_DIALCONF(
	net::Buffer &&packet
) {
	if(conn_state == ConnectionState::DialSent) {
		dst_conn_id = packet.read_uint32_be(2);
		auto src_conn_id = packet.read_uint32_be(6);
		if(src_conn_id != this->src_conn_id) {
			// On conn id mismatch, send RST for that id
			// Connection should ideally be reestablished by
			// dial retry sending another DIAL
			SPDLOG_ERROR(
				"Stream protocol: DIALCONF: Src id mismatch: {}, {}",
				src_conn_id,
				this->src_conn_id
			);
			send_RST(src_conn_id, dst_conn_id);
			dst_conn_id = 0;
			return;
		}

		uv_timer_stop(&state_timer);
		state_timer_interval = 0;

		send_CONF();

		conn_state = ConnectionState::Established;

		if(dialled) {
			delegate->did_dial(*this);
		}
	} else if(conn_state == ConnectionState::DialRcvd) {
		// Usually happend in case of simultaneous open
		auto src_conn_id = packet.read_uint32_be(6);
		auto dst_conn_id = packet.read_uint32_be(2);
		if(src_conn_id != this->src_conn_id || dst_conn_id != this->dst_conn_id) {
			// On conn id mismatch, send RST for that id
			// Connection should ideally be reestablished by
			// dial retry sending another DIAL
			SPDLOG_ERROR(
				"Stream protocol: DIALCONF: Connection id mismatch: {}, {}, {}, {}",
				src_conn_id,
				this->src_conn_id,
				dst_conn_id,
				this->dst_conn_id
			);
			send_RST(src_conn_id, dst_conn_id);
			return;
		}

		uv_timer_stop(&state_timer);
		state_timer_interval = 0;

		send_CONF();

		conn_state = ConnectionState::Established;

		if(dialled) {
			delegate->did_dial(*this);
		}
	} else if(conn_state == ConnectionState::Established) {
		auto src_conn_id = packet.read_uint32_be(6);
		auto dst_conn_id = packet.read_uint32_be(2);
		if(src_conn_id != this->src_conn_id || dst_conn_id != this->dst_conn_id) {
			SPDLOG_ERROR(
				"Stream protocol: DIALCONF: Connection id mismatch: {}, {}, {}, {}",
				src_conn_id,
				this->src_conn_id,
				dst_conn_id,
				this->dst_conn_id
			);
			send_RST(src_conn_id, dst_conn_id);
			return;
		}

		send_CONF();
	} else {
		// Shouldn't receive DIALCONF in other states, unrecoverable
		send_RST(packet.read_uint32_be(6), packet.read_uint32_be(2));
	}
}

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::send_CONF() {
	net::Buffer packet(new char[10] {0, 5}, 10);

	packet.write_uint32_be(2, src_conn_id);
	packet.write_uint32_be(6, dst_conn_id);

	transport.send(std::move(packet));
}

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::did_recv_CONF(
	net::Buffer &&packet
) {
	if(conn_state == ConnectionState::DialRcvd) {
		auto src_conn_id = packet.read_uint32_be(6);
		auto dst_conn_id = packet.read_uint32_be(2);
		if(src_conn_id != this->src_conn_id || dst_conn_id != this->dst_conn_id) {
			send_RST(src_conn_id, dst_conn_id);

			SPDLOG_ERROR(
				"Stream protocol: CONF: Connection id mismatch: {}, {}, {}, {}",
				src_conn_id,
				this->src_conn_id,
				dst_conn_id,
				this->dst_conn_id
			);
			return;
		}

		conn_state = ConnectionState::Established;

		if(dialled) {
			delegate->did_dial(*this);
		}
	} else if(conn_state == ConnectionState::Established) {
		auto src_conn_id = packet.read_uint32_be(6);
		auto dst_conn_id = packet.read_uint32_be(2);
		if(src_conn_id != this->src_conn_id || dst_conn_id != this->dst_conn_id) {
			send_RST(src_conn_id, dst_conn_id);

			SPDLOG_ERROR(
				"Stream protocol: CONF: Connection id mismatch: {}, {}, {}, {}",
				src_conn_id,
				this->src_conn_id,
				dst_conn_id,
				this->dst_conn_id
			);
			return;
		}
	} else {
		// Shouldn't receive CONF in other states, unrecoverable
		send_RST(packet.read_uint32_be(6), packet.read_uint32_be(2));
	}
}

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::send_RST(
	uint32_t src_conn_id,
	uint32_t dst_conn_id
) {
	net::Buffer packet(new char[10] {0, 6}, 10);

	packet.write_uint32_be(2, src_conn_id);
	packet.write_uint32_be(6, dst_conn_id);

	transport.send(std::move(packet));
}

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::did_recv_RST(
	net::Buffer &&packet
) {
	auto src_conn_id = packet.read_uint32_be(6);
	auto dst_conn_id = packet.read_uint32_be(2);
	if(src_conn_id == this->src_conn_id && dst_conn_id == this->dst_conn_id) {
		SPDLOG_ERROR(
			"Stream protocol: RST: {}",
			dst_addr.to_string()
		);
		reset();
	}
}

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::send_DATA(
	net::Buffer &&packet
) {
	transport.send(std::move(packet));
}

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::did_recv_DATA(
	net::Buffer &&packet
) {
	auto &p = *reinterpret_cast<StreamPacket *>(&packet);

	SPDLOG_TRACE("DATA <<< {}: {}, {}", dst_addr.to_string(), p.offset(), p.length());

	auto src_conn_id = p.read_uint32_be(6);
	auto dst_conn_id = p.read_uint32_be(2);
	if(src_conn_id != this->src_conn_id || dst_conn_id != this->dst_conn_id) { // Wrong connection id, send RST
		return;
	}

	auto offset = p.offset();
	auto length = p.length();
	auto packet_number = p.packet_number();

	auto &stream = get_or_create_recv_stream(p.stream_id());

	// Short circuit once stream has been received fully.
	if(stream.state == RecvStream::State::AllRecv ||
		stream.state == RecvStream::State::Read) {
		return;
	}

	// Set stream size if fin bit set
	if(p.is_fin_set() && stream.state == RecvStream::State::Recv) {
		stream.size = offset + length;
		stream.state = RecvStream::State::SizeKnown;
	}

	// Cover header
	p.cover(30);

	// Add to ack range
	ack_ranges.add_packet_number(packet_number);

	// Start ack delay timer if not already active
	if(!ack_timer_active) {
		ack_timer_active = true;
		uv_timer_start(&ack_timer, &ack_timer_cb, 25, 0);
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
		delegate->did_recv_bytes(*this, std::move(packet));
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
				delegate->did_recv_bytes(*this, std::move(packet));
				stream.read_offset = iter->second.offset + iter->second.length;
			}

			// Next iter
			iter = stream.recv_packets.erase(iter);
		}

		// Check all data read
		if(stream.check_read()) {
			stream.state = RecvStream::State::Read;
			recv_streams.erase(stream.stream_id);
		}
	} else {
		// Queue packet for later processing
		stream.recv_packets.try_emplace(
			offset,
			uv_now(uv_default_loop()),
			offset,
			length,
			std::move(p)
		);

		// Check all data received
		if (stream.check_finish()) {
			stream.state = RecvStream::State::AllRecv;
		}
	}
}

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::send_ACK() {
	net::Buffer packet(new char[508] {0, 2}, 508);

	int size = ack_ranges.ranges.size() > 61 ? 61 : ack_ranges.ranges.size();

	packet.write_uint32_be(2, src_conn_id);
	packet.write_uint32_be(6, dst_conn_id);
	packet.write_uint16_be(10, size);
	packet.write_uint64_be(12, ack_ranges.largest);

	int i = 20;
	auto iter = ack_ranges.ranges.begin();
	// Upto 30 gaps
	for(
		;
		i <= 500 && iter != ack_ranges.ranges.end();
		i += 8, iter++
	) {
		packet.write_uint64_be(i, *iter);
	}

	transport.send(std::move(packet));
}

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::did_recv_ACK(
	net::Buffer &&packet
) {
	SPDLOG_TRACE("ACK <<< {}", dst_addr.to_string());

	auto &p = *reinterpret_cast<StreamPacket *>(&packet);

	auto src_conn_id = p.read_uint32_be(6);
	auto dst_conn_id = p.read_uint32_be(2);
	if(src_conn_id != this->src_conn_id || dst_conn_id != this->dst_conn_id) { // Wrong connection id, send RST
		return;
	}

	auto now = uv_now(uv_default_loop());

	// TODO: Better method names
	uint16_t size = p.stream_id();
	uint64_t largest = p.packet_number();

	// New largest acked packet
	if(largest > largest_acked && sent_packets.find(largest) != sent_packets.end()) {
		auto &sent_packet = sent_packets[largest];

		// Update largest packet details
		largest_acked = largest;
		largest_sent_time = sent_packet.sent_time;

		// Update RTT estimate
		if(rtt < 0) {
			rtt = now - sent_packet.sent_time;
		} else {
			rtt = 0.875 * rtt + 0.125 * (now - sent_packet.sent_time);
		}
	}

	// Cover till range list
	p.cover(20);

	uint64_t high = largest;
	bool gap = false;
	bool is_app_limited = (bytes_in_flight < 0.8 * congestion_window);

	for(
		uint16_t i = 0;
		i < size;
		i++, gap = !gap
	) {
		uint64_t range = p.read_uint64_be(i*8);

		uint64_t low = high - range;

		// Short circuit on gap range
		if(gap) {
			high = low;
			continue;
		}

		// Get packets within range [low+1, high]
		auto low_iter = sent_packets.lower_bound(low + 1);
		auto high_iter = sent_packets.upper_bound(high);

		// Iterate acked packets
		for(
			;
			low_iter != high_iter;
			low_iter = sent_packets.erase(low_iter)
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

					delegate->did_send_bytes(
						*this,
						net::Buffer(
							iter->data.release(),
							iter->size
						)
					);
				}
			} else {
				// Already acked range, ignore
			}

			// Cleanup
			stream.bytes_in_flight -= sent_packet.length;
			bytes_in_flight -= sent_packet.length;

			SPDLOG_TRACE("Times: {}, {}", sent_packet.sent_time, congestion_start);

			// Congestion control
			// Check if not in congestion recovery and not application limited
			if(sent_packet.sent_time > congestion_start && !is_app_limited) {
				if(congestion_window < ssthresh) {
					// Slow start, exponential increase
					congestion_window += sent_packet.length;
				} else {
					// Congestion avoidance, CUBIC
					// auto k = k;
					// auto t = now - congestion_start;
					// congestion_window = w_max + 4 * std::pow(0.001 * (t - k), 3);

					// if(congestion_window < 10000) {
					// 	congestion_window = 10000;
					// }

					// Congestion avoidance, NEW RENO
					congestion_window += 1500 * sent_packet.length / congestion_window;
				}
			}

			// Check stream finish
			if (stream.state == SendStream::State::Sent &&
				stream.acked_offset == stream.queue_offset) {
				stream.state = SendStream::State::Acked;

				// TODO: Call delegate to inform
				SPDLOG_DEBUG("Acked: {}", stream.stream_id);

				// Remove stream
				send_streams.erase(stream.stream_id);

				return;
			}
		}

		high = low;
	}

	auto sent_iter = sent_packets.cbegin();

	// Determine lost packets
	while(sent_iter != sent_packets.cend()) {
		// Condition for packet in flight to be considered lost
		// 1. more than 20 packets before largest acked - disabled for now
		// 2. more than 25ms before before largest acked
		if (/*sent_iter->first + 20 < largest_acked ||*/
			largest_sent_time > sent_iter->second.sent_time + 25) {

			bytes_in_flight -= sent_iter->second.length;
			sent_iter->second.stream->bytes_in_flight -= sent_iter->second.length;
			lost_packets[sent_iter->first] = sent_iter->second;

			sent_iter++;
		} else {
			break;
		}
	}

	if(sent_iter == sent_packets.cbegin()) {
		// No lost packets, ignore
	} else {
		// Lost packets, congestion event
		auto last_iter = std::prev(sent_iter);
		auto &sent_packet = last_iter->second;

		if(sent_packet.sent_time > congestion_start) {
			// New congestion event
			SPDLOG_ERROR("Stream protocol: Congestion event: {}", congestion_window);
			congestion_start = now;

			if(congestion_window < w_max) {
				// Fast convergence
				w_max = congestion_window;
				congestion_window *= 0.6;
			} else {
				w_max = congestion_window;
				congestion_window *= 0.75;
			}

			if(congestion_window < 10000) {
				congestion_window = 10000;
			}
			ssthresh = congestion_window;
			k = std::cbrt(w_max / 16)*1000;
		}

		// Pop lost packets from sent
		sent_packets.erase(sent_packets.cbegin(), sent_iter);
	}

	// New packets
	send_pending_data();

	tlp_interval = DEFAULT_TLP_INTERVAL;
	uv_timer_start(&tlp_timer, &tlp_timer_cb, tlp_interval, 0);
}

//---------------- Protocol functions end ----------------//


//---------------- Delegate functions begin ----------------//

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::did_dial(
	BaseTransport &
) {
	// Begin handshake
	dialled = true;

	state_timer_interval = 1000;
	uv_timer_start(&state_timer, dial_timer_cb, state_timer_interval, 0);

	src_conn_id = (uint32_t)std::random_device()();
	send_DIAL();
	conn_state = ConnectionState::DialSent;
}

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::did_recv_packet(
	BaseTransport &,
	net::Buffer &&packet
) {
	switch(packet.read_uint8(1)) {
		// DATA
		case 0:
		// DATA + FIN
		case 1: did_recv_DATA(std::move(packet));
		break;
		// ACK
		case 2: did_recv_ACK(std::move(packet));
		break;
		// DIAL
		case 3: did_recv_DIAL(std::move(packet));
		break;
		// DIALCONF
		case 4: did_recv_DIALCONF(std::move(packet));
		break;
		// CONF
		case 5: did_recv_CONF(std::move(packet));
		break;
		// RST
		case 6: did_recv_RST(std::move(packet));
		break;
		// UNKNOWN
		default: SPDLOG_TRACE("UNKNOWN <<< {}", dst_addr.to_string());
		break;
	}
}

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::did_send_packet(
	BaseTransport &,
	net::Buffer &&packet
) {
	switch(packet.read_uint8(1)) {
		// DATA
		case 0: SPDLOG_TRACE("DATA >>> {}", dst_addr.to_string());
		break;
		// DATA + FIN
		case 1: SPDLOG_TRACE("DATA + FIN >>> {}", dst_addr.to_string());
		break;
		// ACK
		case 2: SPDLOG_TRACE("ACK >>> {}", dst_addr.to_string());
		break;
		// DIAL
		case 3: SPDLOG_TRACE("DIAL >>> {}", dst_addr.to_string());
		break;
		// DIALCONF
		case 4: SPDLOG_TRACE("DIALCONF >>> {}", dst_addr.to_string());
		break;
		// CONF
		case 5: SPDLOG_TRACE("CONF >>> {}", dst_addr.to_string());
		break;
		// RST
		case 6: SPDLOG_TRACE("RST >>> {}", dst_addr.to_string());
		break;
		// UNKNOWN
		default: SPDLOG_TRACE("UNKNOWN >>> {}", dst_addr.to_string());
		break;
	}
}

//---------------- Delegate functions end ----------------//

template<typename DelegateType, template<typename> class DatagramTransport>
StreamTransport<DelegateType, DatagramTransport>::StreamTransport(
	net::SocketAddress const &src_addr,
	net::SocketAddress const &dst_addr,
	BaseTransport &transport,
	net::TransportManager<StreamTransport<DelegateType, DatagramTransport>> &transport_manager
) : transport(transport), transport_manager(transport_manager), src_addr(src_addr), dst_addr(dst_addr) {
	uv_timer_init(uv_default_loop(), &this->state_timer);
	this->state_timer.data = this;

	uv_timer_init(uv_default_loop(), &this->tlp_timer);
	this->tlp_timer.data = this;

	uv_timer_init(uv_default_loop(), &this->ack_timer);
	this->ack_timer.data = this;

	uv_timer_init(uv_default_loop(), &this->pacing_timer);
	this->pacing_timer.data = this;
}

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::setup(
	DelegateType *delegate
) {
	this->delegate = delegate;

	transport.setup(this);
}

template<typename DelegateType, template<typename> class DatagramTransport>
int StreamTransport<DelegateType, DatagramTransport>::send(
	net::Buffer &&bytes
) {
	uint8_t stream_id = 0;
	auto &stream = get_or_create_send_stream(stream_id);

	if(stream.state == SendStream::State::Ready) {
		stream.state = SendStream::State::Send;
	}

	// Abort if data queue is too big
	if(stream.queue_offset - stream.acked_offset > 20000000)
		return -1;

	auto size = bytes.size();

	// Add data to send queue
	stream.data_queue.emplace_back(
		std::move(bytes),
		size,
		stream.queue_offset
	);

	stream.queue_offset += size;

	// Handle idle stream
	if(stream.next_item_iterator == stream.data_queue.end()) {
		stream.next_item_iterator = std::prev(stream.data_queue.end());
	}

	// Handle idle connection
	if(sent_packets.size() == 0 && lost_packets.size() == 0 && send_queue.size() == 0) {
		uv_timer_start(&tlp_timer, &tlp_timer_cb, tlp_interval, 0);
	}

	register_send_intent(stream);
	send_pending_data();

	return 0;
}

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::close() {
	reset();
	transport_manager.erase(dst_addr);
	transport.close();
}

template<typename DelegateType, template<typename> class DatagramTransport>
bool StreamTransport<DelegateType, DatagramTransport>::is_active() {
	if(conn_state == ConnectionState::Established) {
		return true;
	}

	return false;
}

} // namespace stream
} // namespace marlin

#endif // MARLIN_STREAM_STREAMTRANSPORT_HPP
