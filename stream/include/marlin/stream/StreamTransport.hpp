/*! \file StreamTransport.hpp
	\brief Building on UDP
*/


#ifndef MARLIN_STREAM_STREAMTRANSPORT_HPP
#define MARLIN_STREAM_STREAMTRANSPORT_HPP

#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <unordered_set>
#include <unordered_map>
#include <random>
#include <utility>

#include <sodium.h>

#include <marlin/core/SocketAddress.hpp>
#include <marlin/core/Buffer.hpp>
#include <marlin/asyncio/core/EventLoop.hpp>
#include <marlin/asyncio/core/Timer.hpp>
#include <marlin/core/TransportManager.hpp>

#include "protocol/SendStream.hpp"
#include "protocol/RecvStream.hpp"
#include "protocol/AckRanges.hpp"
#include "Messages.hpp"

namespace marlin {
namespace stream {

#define DEFAULT_TLP_INTERVAL 1000
#define DEFAULT_PACING_LIMIT 25000
#define DEFAULT_FRAGMENT_SIZE (1400 - crypto_aead_aes256gcm_NPUBBYTES)

//! Custom transport class building upon UDP
/*!
	A higher order transport (HOT) building over Marlin UDPTransport

	Features:
	\li 3 way handshake for connection establishment
	\li supports multiple streams
	\li FEC support to be integrated
*/
template<typename DelegateType, template<typename> class DatagramTransport>
class StreamTransport {
public:
	static constexpr bool is_encrypted = true;
private:
	using Self = StreamTransport<DelegateType, DatagramTransport>;
	using BaseTransport = DatagramTransport<Self>;

	BaseTransport &transport;
	core::TransportManager<Self> &transport_manager;

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
	asyncio::Timer state_timer;
	uint64_t state_timer_interval = 0;

	void dial_timer_cb();

	// Streams
	std::unordered_map<uint16_t, SendStream> send_streams;
	std::unordered_map<uint16_t, RecvStream> recv_streams;

	SendStream &get_or_create_send_stream(uint16_t const stream_id);
	RecvStream &get_or_create_recv_stream(uint16_t const stream_id);

	// Packets

	/*!
		strictly increasing packet number so that all packets including the lost ones which are being retransmitted have unique packet number
	*/
	uint64_t last_sent_packet = -1;
	std::map<uint64_t, SentPacketInfo> sent_packets;

	/*!
		packets which not have been acked since long time
	*/
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
	asyncio::Timer pacing_timer;
	bool is_pacing_timer_active = false;

	void pacing_timer_cb();

	// TLP
	asyncio::Timer tlp_timer;
	uint64_t tlp_interval = DEFAULT_TLP_INTERVAL;

	void tlp_timer_cb();

	// ACKs
	AckRanges ack_ranges;
	asyncio::Timer ack_timer;
	bool ack_timer_active = false;

	void ack_timer_cb();

	// Protocol
	void send_DIAL();
	void did_recv_DIAL(DIAL &&packet);

	void send_DIALCONF();
	void did_recv_DIALCONF(DIALCONF &&packet);

	void send_CONF();
	void did_recv_CONF(CONF &&packet);

	void send_RST(uint32_t src_conn_id, uint32_t dst_conn_id);
	void did_recv_RST(core::Buffer &&packet);

	void send_DATA(core::Buffer &&packet);
	void did_recv_DATA(core::Buffer &&packet);

	void send_ACK();
	void did_recv_ACK(core::Buffer &&packet);

	void send_SKIPSTREAM(uint16_t stream_id, uint64_t offset);
	void did_recv_SKIPSTREAM(core::Buffer &&packet);

	void send_FLUSHSTREAM(uint16_t stream_id, uint64_t offset);
	void did_recv_FLUSHSTREAM(core::Buffer &&packet);

	void send_FLUSHCONF(uint16_t stream_id);
	void did_recv_FLUSHCONF(core::Buffer &&packet);

public:
	// Delegate
	void did_dial(BaseTransport &transport);
	void did_recv_packet(BaseTransport &transport, core::Buffer &&packet);
	void did_send_packet(BaseTransport &transport, core::Buffer &&packet);
	void did_close(BaseTransport &transport);

	core::SocketAddress src_addr;
	core::SocketAddress dst_addr;

	DelegateType *delegate = nullptr;

	StreamTransport(
		core::SocketAddress const &src_addr,
		core::SocketAddress const &dst_addr,
		BaseTransport &transport,
		core::TransportManager<Self> &transport_manager,
		uint8_t const* remote_static_pk
	);

	void setup(DelegateType *delegate, uint8_t const* static_sk);
	int send(core::Buffer &&bytes, uint16_t stream_id = 0);
	void close();

	bool is_active();
	double get_rtt();

	void skip_timer_cb(RecvStream& stream);
	void skip_stream(uint16_t stream_id);

	void flush_timer_cb(SendStream& stream);
	void flush_stream(uint16_t stream_id);

private:
	uint8_t static_sk[crypto_box_SECRETKEYBYTES];
	uint8_t static_pk[crypto_box_PUBLICKEYBYTES];

	uint8_t remote_static_pk[crypto_box_PUBLICKEYBYTES];

	uint8_t ephemeral_sk[crypto_kx_SECRETKEYBYTES];
	uint8_t ephemeral_pk[crypto_kx_PUBLICKEYBYTES];

	uint8_t remote_ephemeral_pk[crypto_kx_PUBLICKEYBYTES];

	uint8_t rx[crypto_kx_SESSIONKEYBYTES];
	uint8_t tx[crypto_kx_SESSIONKEYBYTES];

	uint8_t rx_IV[crypto_aead_aes256gcm_NPUBBYTES];
	uint8_t tx_IV[crypto_aead_aes256gcm_NPUBBYTES];

	alignas(16) crypto_aead_aes256gcm_state rx_ctx;
	alignas(16) crypto_aead_aes256gcm_state tx_ctx;
public:
	uint8_t const* get_static_pk();
	uint8_t const* get_remote_static_pk();
};


// Impl

/*!
	Resets and clears connection-state, timers, streams and various other tranport related params
*/
template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::reset() {
	// Reset transport
	conn_state = ConnectionState::Listen;
	src_conn_id = 0;
	dst_conn_id = 0;
	dialled = false;
	state_timer.stop();
	state_timer_interval = 0;

	for(auto& [_, stream] : send_streams) {
		stream.state_timer.stop();
	}
	send_streams.clear();
	for(auto& [_, stream] : recv_streams) {
		stream.state_timer.stop();
	}
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

	pacing_timer.stop();
	is_pacing_timer_active = false;

	tlp_timer.stop();
	tlp_interval = DEFAULT_TLP_INTERVAL;

	ack_ranges = AckRanges();
	ack_timer.stop();
	ack_timer_active = false;
}

// Impl

//! a callback function which sends dial message with exponential interval increases
template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::dial_timer_cb() {
	if(this->state_timer_interval >= 64000) { // Abort on too many retries
		this->state_timer_interval = 0;
		SPDLOG_ERROR(
			"Stream transport {{ Src: {}, Dst: {} }}: Dial timeout",
			this->src_addr.to_string(),
			this->dst_addr.to_string()
		);
		this->close();
		return;
	}

	this->send_DIAL();
	this->state_timer_interval *= 2;
	this->state_timer.template start<Self, &Self::dial_timer_cb>(
		this->state_timer_interval,
		0
	);
}

//---------------- Stream functions begin ----------------//


//! creates or return sendstream of given stream_id
/*!
	Takes in the stream_id and returns the SendStream corresponding to it. Creates and return if none found.

	\param stream_id stream id to search for
	\return SendStream object corresponding for given param stream_id
*/
template<typename DelegateType, template<typename> class DatagramTransport>
SendStream &StreamTransport<DelegateType, DatagramTransport>::get_or_create_send_stream(
	uint16_t const stream_id
) {
	auto iter = send_streams.try_emplace(
		stream_id,
		stream_id,
		this
	).first;

	return iter->second;
}


//! creates or return recvstream of given stream_id
/*!
	Takes in the stream_id and returns the RecvStream corresponding to it. Creates and return if none found.

	\param stream_id stream id to search for
	\return RecvStream object corresponding for given param stream_id
*/
template<typename DelegateType, template<typename> class DatagramTransport>
RecvStream &StreamTransport<DelegateType, DatagramTransport>::get_or_create_recv_stream(
	uint16_t const stream_id
) {
	auto iter = recv_streams.try_emplace(
		stream_id,
		stream_id,
		this
	).first;

	return iter->second;
}

//---------------- Stream functions end ----------------//


//---------------- Send functions end ----------------//


//! adds given SendStream to send_queue
/*!
	Takes in the stream and adds it to the queue of streams which are inspected for data packets during the sending phase

	\param stream object to be added to the queue
	\return false if stream already in queue otherwise true
*/
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

//! Sends across given data item fragment
/*!
	\param stream SendStream to which the fragment's dataItem belongs to
	\param data_item DataItem to which the fragment belongs to
	\param offset integer offset of the fragment in the data_item
	\param length byte length of the fragment
*/
template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::send_data_packet(
	SendStream &stream,
	DataItem &data_item,
	uint64_t offset,
	uint16_t length
) {
	bool is_fin = (stream.done_queueing &&
		data_item.stream_offset + offset + length >= stream.queue_offset);

	core::Buffer packet(
		{0, static_cast<uint8_t>(is_fin)},
		length + 30 + crypto_aead_aes256gcm_ABYTES
	);

	this->last_sent_packet++;

	packet.write_uint32_be_unsafe(2, src_conn_id);
	packet.write_uint32_be_unsafe(6, dst_conn_id);
	packet.write_uint16_be_unsafe(10, stream.stream_id);
	packet.write_uint64_be_unsafe(12, this->last_sent_packet);
	packet.write_uint64_be_unsafe(20, data_item.stream_offset + offset);
	packet.write_uint16_be_unsafe(28, length);
	packet.write_unsafe(30, data_item.data.data()+offset, length);

	uint8_t nonce[12];
	for(int i = 0; i < 4; i++) {
		nonce[i] = tx_IV[i] ^ packet.data()[2+i];
	}
	for(int i = 0; i < 8; i++) {
		nonce[4+i] = tx_IV[4+i] ^ packet.data()[12+i];
	}
	crypto_aead_aes256gcm_encrypt_afternm(
		packet.data() + 20,
		nullptr,
		packet.data() + 20,
		10 + length,
		packet.data() + 2,
		18,
		nullptr,
		nonce,
		&tx_ctx
	);

	this->sent_packets.emplace(
		std::piecewise_construct,
		std::forward_as_tuple(this->last_sent_packet),
		std::forward_as_tuple(
			asyncio::EventLoop::now(),
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

//! Schedules pacing timer callback
template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::send_pending_data() {
	if(is_pacing_timer_active == false) {
		is_pacing_timer_active = true;
		pacing_timer.template start<Self, &Self::pacing_timer_cb>(0, 0);
	}
}

//! Attempts to send again the lost packets
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
			pacing_timer.template start<Self, &Self::pacing_timer_cb>(1, 0);
			return -1;
		}

		auto &sent_packet = iter->second;
		if(bytes_in_flight > congestion_window - sent_packet.length) {
			return -2;
		}

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

//! Attempts to send new packets in the given stream
/*!
	fragments dataItems in the stream and sends them across until congestion or pacing limit is not hit
	\param stream SendStream from which DataItems/Packets needs to be dequeued for sending
	\param initial_bytes_in_flight for the purpose of keeping a check on congestion and pacing limit
*/
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
			i < data_item.data.size();
			i+=DEFAULT_FRAGMENT_SIZE
		) {
			auto remaining_bytes = data_item.data.size() - data_item.sent_offset;
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
void StreamTransport<DelegateType, DatagramTransport>::pacing_timer_cb() {
	this->is_pacing_timer_active = false;

	auto initial_bytes_in_flight = this->bytes_in_flight;

	auto res = this->send_lost_data(initial_bytes_in_flight);
	if(res < 0) {
		return;
	}

	// New packets
	for(
		auto iter = this->send_queue.begin();
		iter != this->send_queue.end();
		// Empty
	) {
		auto &stream = **iter;

		int res = this->send_new_data(stream, initial_bytes_in_flight);
		if(res == 0) { // Idle stream, move to next stream
			this->send_queue_ids.erase(stream.stream_id);
			iter = this->send_queue.erase(iter);
		} else if(res == -1) { // Pacing limit hit, reschedule timer
			// Pacing limit hit, reschedule timer
			this->is_pacing_timer_active = true;
			pacing_timer.template start<Self, &Self::pacing_timer_cb>(1, 0);
			return;
		} else { // Congestion window exhausted, break
			return;
		}
	}
}

//---------------- Pacing functions end ----------------//


//---------------- TLP functions begin ----------------//

//! tail loss probe (tlp) function to detect inactivity
/*!
	called at TLP_INTERVAL to detect inactivity. examines for packet losses and adjusts congestion window accordingly
*/

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::tlp_timer_cb() {
	if(this->sent_packets.size() == 0 && this->lost_packets.size() == 0 && this->send_queue.size() == 0) {
		// Idle connection, stop timer
		tlp_timer.stop();
		this->tlp_interval = DEFAULT_TLP_INTERVAL;
		return;
	}

	auto sent_iter = this->sent_packets.cbegin();

	// Retry lost packets
	// No condition necessary, all are considered lost if tail probe fails
	while(sent_iter != this->sent_packets.cend()) {
		this->bytes_in_flight -= sent_iter->second.length;
		sent_iter->second.stream->bytes_in_flight -= sent_iter->second.length;
		this->lost_packets[sent_iter->first] = sent_iter->second;

		sent_iter++;
	}

	if(sent_iter == this->sent_packets.cbegin()) {
		// No lost packets, ignore
	} else {
		// Lost packets, congestion event
		auto last_iter = std::prev(sent_iter);
		auto &sent_packet = last_iter->second;
		if(sent_packet.sent_time > this->congestion_start) {
			// New congestion event
			SPDLOG_ERROR(
				"Stream transport {{ Src: {}, Dst: {} }}: Timer congestion event: {}",
				this->src_addr.to_string(),
				this->dst_addr.to_string(),
				this->congestion_window
			);
			this->congestion_start = asyncio::EventLoop::now();

			if(this->congestion_window < this->w_max) {
				// Fast convergence
				this->w_max = this->congestion_window;
				this->congestion_window *= 0.6;
			} else {
				this->w_max = this->congestion_window;
				this->congestion_window *= 0.75;
			}

			if(this->congestion_window < 10000) {
				this->congestion_window = 10000;
			}

			this->ssthresh = this->congestion_window;

			this->k = std::cbrt(this->w_max / 16)*1000;
		}

		// Pop lost packets from sent
		this->sent_packets.erase(this->sent_packets.cbegin(), sent_iter);
	}

	// New packets
	this->send_pending_data();

	// Next timer interval
	if(this->tlp_interval < 25000) {
		this->tlp_interval *= 2;
		this->tlp_timer.template start<Self, &Self::tlp_timer_cb>(this->tlp_interval, 0);
	} else {
		// Abort on too many retries
		SPDLOG_ERROR("Lost peer: {}", this->dst_addr.to_string());
		this->close();
	}
}

//---------------- TLP functions end ----------------//


//---------------- ACK functions begin ----------------//

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::ack_timer_cb() {
	send_ACK();

	ack_timer_active = false;
}

//---------------- ACK functions end ----------------//


//---------------- Protocol functions begin ----------------//

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::send_DIAL() {
	SPDLOG_DEBUG(
		"Stream transport {{ Src: {}, Dst: {} }}: DIAL >>>> {:spn}",
		src_addr.to_string(),
		dst_addr.to_string(),
		spdlog::to_hex(remote_static_pk, remote_static_pk+crypto_box_PUBLICKEYBYTES)
	);

	constexpr size_t pt_len = crypto_box_PUBLICKEYBYTES + crypto_kx_PUBLICKEYBYTES;
	constexpr size_t ct_len = pt_len + crypto_box_SEALBYTES;

	// The plaintext needs to leave at least 32 bytes in the beginning
	// since the ephemeral key is written first before encryption
	// TODO: See if we can get this changed upstream
	uint8_t buf[ct_len];
	std::memcpy(buf + 32, static_pk, crypto_box_PUBLICKEYBYTES);
	std::memcpy(buf + 32 + crypto_box_PUBLICKEYBYTES, ephemeral_pk, crypto_kx_PUBLICKEYBYTES);
	crypto_box_seal(buf, buf + 32, pt_len, remote_static_pk);

	transport.send(DIAL(this->src_conn_id, this->dst_conn_id, buf, ct_len));
}

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::did_recv_DIAL(
	DIAL &&packet
) {
	constexpr size_t pt_len = (crypto_box_PUBLICKEYBYTES + crypto_kx_PUBLICKEYBYTES);
	constexpr size_t ct_len = pt_len + crypto_box_SEALBYTES;

	if(!packet.validate(ct_len)) {
		return;
	}

	if(conn_state == ConnectionState::Listen) {
		if(packet.src_conn_id() != 0) { // Should have empty source
			SPDLOG_ERROR(
				"Stream transport {{ Src: {}, Dst: {} }}: DIAL: Should have empty src: {}",
				src_addr.to_string(),
				dst_addr.to_string(),
				packet.src_conn_id()
			);
			return;
		}

		SPDLOG_DEBUG(
			"Stream transport {{ Src: {}, Dst: {} }}: DIAL <<<< {:spn}",
			src_addr.to_string(),
			dst_addr.to_string(),
			spdlog::to_hex(static_pk, static_pk+crypto_box_PUBLICKEYBYTES)
		);

		uint8_t pt[pt_len];
		auto res = crypto_box_seal_open(pt, packet.payload(), ct_len, static_pk, static_sk);
		if (res < 0) {
			SPDLOG_ERROR(
				"Stream transport {{ Src: {}, Dst: {} }}: DIAL: Unseal failure: {}",
				src_addr.to_string(),
				dst_addr.to_string(),
				res
			);
			return;
		}

		std::memcpy(remote_static_pk, pt, crypto_box_PUBLICKEYBYTES);
		std::memcpy(remote_ephemeral_pk, pt + crypto_box_PUBLICKEYBYTES, crypto_kx_PUBLICKEYBYTES);

		auto *kdf = (std::memcmp(ephemeral_pk, remote_ephemeral_pk, crypto_box_PUBLICKEYBYTES) > 0) ? &crypto_kx_server_session_keys : &crypto_kx_client_session_keys;

		if ((*kdf)(rx, tx, ephemeral_pk, ephemeral_sk, remote_ephemeral_pk) != 0) {
			SPDLOG_ERROR(
				"Stream transport {{ Src: {}, Dst: {} }}: DIAL: Key derivation failure",
				src_addr.to_string(),
				dst_addr.to_string()
			);
			return;
		}

		randombytes_buf_deterministic(rx_IV, crypto_aead_aes256gcm_NPUBBYTES, rx);
		randombytes_buf_deterministic(tx_IV, crypto_aead_aes256gcm_NPUBBYTES, tx);
		crypto_aead_aes256gcm_beforenm(&rx_ctx, rx);
		crypto_aead_aes256gcm_beforenm(&tx_ctx, tx);

		this->dst_conn_id = packet.dst_conn_id();
		this->src_conn_id = (uint32_t)std::random_device()();

		send_DIALCONF();

		conn_state = ConnectionState::DialRcvd;
	} else if(conn_state == ConnectionState::DialSent) {
		if(packet.src_conn_id() != 0) { // Should have empty source
			SPDLOG_ERROR(
				"Stream transport {{ Src: {}, Dst: {} }}: DIAL: Should have empty src: {}",
				src_addr.to_string(),
				dst_addr.to_string(),
				packet.src_conn_id()
			);
			return;
		}

		uint8_t pt[pt_len];
		auto res = crypto_box_seal_open(pt, packet.data() + 10, ct_len, static_pk, static_sk);
		if (res < 0) {
			SPDLOG_ERROR(
				"Stream transport {{ Src: {}, Dst: {} }}: DIAL: Unseal failure: {}",
				src_addr.to_string(),
				dst_addr.to_string(),
				res
			);
			return;
		}

		std::memcpy(remote_ephemeral_pk, pt + crypto_box_PUBLICKEYBYTES, crypto_kx_PUBLICKEYBYTES);

		auto *kdf = (std::memcmp(ephemeral_pk, remote_ephemeral_pk, crypto_box_PUBLICKEYBYTES) > 0) ? &crypto_kx_server_session_keys : &crypto_kx_client_session_keys;

		if ((*kdf)(rx, tx, ephemeral_pk, ephemeral_sk, remote_ephemeral_pk) != 0) {
			SPDLOG_ERROR(
				"Stream transport {{ Src: {}, Dst: {} }}: DIAL: Key derivation failure",
				src_addr.to_string(),
				dst_addr.to_string()
			);
			return;
		}

		randombytes_buf_deterministic(rx_IV, crypto_aead_aes256gcm_NPUBBYTES, rx);
		randombytes_buf_deterministic(tx_IV, crypto_aead_aes256gcm_NPUBBYTES, tx);
		crypto_aead_aes256gcm_beforenm(&rx_ctx, rx);
		crypto_aead_aes256gcm_beforenm(&tx_ctx, tx);

		this->dst_conn_id = packet.dst_conn_id();

		state_timer.stop();
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
	constexpr size_t pt_len = crypto_kx_PUBLICKEYBYTES;
	constexpr size_t ct_len = pt_len + crypto_box_SEALBYTES;

	uint8_t buf[ct_len];
	crypto_box_seal(buf, ephemeral_pk, pt_len, remote_static_pk);

	transport.send(DIALCONF(this->src_conn_id, this->dst_conn_id, buf, ct_len));
}

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::did_recv_DIALCONF(
	DIALCONF &&packet
) {
	constexpr size_t pt_len = crypto_kx_PUBLICKEYBYTES;
	constexpr size_t ct_len = pt_len + crypto_box_SEALBYTES;

	if(!packet.validate(ct_len)) {
		return;
	}

	if(conn_state == ConnectionState::DialSent) {
		auto src_conn_id = packet.src_conn_id();
		if(src_conn_id != this->src_conn_id) {
			// On conn id mismatch, send RST for that id
			// Connection should ideally be reestablished by
			// dial retry sending another DIAL
			SPDLOG_ERROR(
				"Stream transport {{ Src: {}, Dst: {} }}: DIALCONF: Src id mismatch: {}, {}",
				src_addr.to_string(),
				dst_addr.to_string(),
				src_conn_id,
				this->src_conn_id
			);
			send_RST(src_conn_id, packet.dst_conn_id());
			return;
		}

		if (crypto_box_seal_open(remote_ephemeral_pk, packet.payload(), ct_len, static_pk, static_sk) != 0) {
			SPDLOG_ERROR(
				"Stream transport {{ Src: {}, Dst: {} }}: DIALCONF: Unseal failure",
				src_addr.to_string(),
				dst_addr.to_string()
			);
			return;
		}

		auto *kdf = (std::memcmp(ephemeral_pk, remote_ephemeral_pk, crypto_box_PUBLICKEYBYTES) > 0) ? &crypto_kx_server_session_keys : &crypto_kx_client_session_keys;

		if ((*kdf)(rx, tx, ephemeral_pk, ephemeral_sk, remote_ephemeral_pk) != 0) {
			SPDLOG_ERROR(
				"Stream transport {{ Src: {}, Dst: {} }}: DIALCONF: Key derivation failure",
				src_addr.to_string(),
				dst_addr.to_string()
			);
			return;
		}

		randombytes_buf_deterministic(rx_IV, crypto_aead_aes256gcm_NPUBBYTES, rx);
		randombytes_buf_deterministic(tx_IV, crypto_aead_aes256gcm_NPUBBYTES, tx);
		crypto_aead_aes256gcm_beforenm(&rx_ctx, rx);
		crypto_aead_aes256gcm_beforenm(&tx_ctx, tx);

		state_timer.stop();
		state_timer_interval = 0;

		this->dst_conn_id = packet.dst_conn_id();

		send_CONF();

		conn_state = ConnectionState::Established;

		if(dialled) {
			delegate->did_dial(*this);
		}
	} else if(conn_state == ConnectionState::DialRcvd) {
		// Usually happend in case of simultaneous open
		auto src_conn_id = packet.src_conn_id();
		auto dst_conn_id = packet.dst_conn_id();
		if(src_conn_id != this->src_conn_id || dst_conn_id != this->dst_conn_id) {
			// On conn id mismatch, send RST for that id
			// Connection should ideally be reestablished by
			// dial retry sending another DIAL
			SPDLOG_ERROR(
				"Stream transport {{ Src: {}, Dst: {} }}: DIALCONF: Connection id mismatch: {}, {}, {}, {}",
				src_addr.to_string(),
				dst_addr.to_string(),
				src_conn_id,
				this->src_conn_id,
				dst_conn_id,
				this->dst_conn_id
			);
			send_RST(src_conn_id, dst_conn_id);
			return;
		}

		state_timer.stop();
		state_timer_interval = 0;

		send_CONF();

		conn_state = ConnectionState::Established;

		if(dialled) {
			delegate->did_dial(*this);
		}
	} else if(conn_state == ConnectionState::Established) {
		auto src_conn_id = packet.src_conn_id();
		auto dst_conn_id = packet.dst_conn_id();
		if(src_conn_id != this->src_conn_id || dst_conn_id != this->dst_conn_id) {
			SPDLOG_ERROR(
				"Stream transport {{ Src: {}, Dst: {} }}: DIALCONF: Connection id mismatch: {}, {}, {}, {}",
				src_addr.to_string(),
				dst_addr.to_string(),
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
		SPDLOG_ERROR(
			"Stream transport {{ Src: {}, Dst: {} }}: DIALCONF: Unexpected",
			src_addr.to_string(),
			dst_addr.to_string()
		);
		send_RST(packet.src_conn_id(), packet.dst_conn_id());
	}
}

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::send_CONF() {
	transport.send(CONF(this->src_conn_id, this->dst_conn_id));
}

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::did_recv_CONF(
	CONF &&packet
) {
	if(!packet.validate()) {
		return;
	}

	if(conn_state == ConnectionState::DialRcvd) {
		auto src_conn_id = packet.src_conn_id();
		auto dst_conn_id = packet.dst_conn_id();
		if(src_conn_id != this->src_conn_id || dst_conn_id != this->dst_conn_id) {
			SPDLOG_ERROR(
				"Stream transport {{ Src: {}, Dst: {} }}: CONF: Connection id mismatch: {}, {}, {}, {}",
				src_addr.to_string(),
				dst_addr.to_string(),
				src_conn_id,
				this->src_conn_id,
				dst_conn_id,
				this->dst_conn_id
			);
			send_RST(src_conn_id, dst_conn_id);

			return;
		}

		conn_state = ConnectionState::Established;

		if(dialled) {
			delegate->did_dial(*this);
		}
	} else if(conn_state == ConnectionState::Established) {
		auto src_conn_id = packet.src_conn_id();
		auto dst_conn_id = packet.dst_conn_id();
		if(src_conn_id != this->src_conn_id || dst_conn_id != this->dst_conn_id) {
			SPDLOG_ERROR(
				"Stream transport {{ Src: {}, Dst: {} }}: CONF: Connection id mismatch: {}, {}, {}, {}",
				src_addr.to_string(),
				dst_addr.to_string(),
				src_conn_id,
				this->src_conn_id,
				dst_conn_id,
				this->dst_conn_id
			);
			send_RST(src_conn_id, dst_conn_id);

			return;
		}
	} else {
		// Shouldn't receive CONF in other states, unrecoverable
		SPDLOG_ERROR(
			"Stream transport {{ Src: {}, Dst: {} }}: CONF: Unexpected",
			src_addr.to_string(),
			dst_addr.to_string()
		);
		send_RST(packet.src_conn_id(), packet.dst_conn_id());
	}
}

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::send_RST(
	uint32_t src_conn_id,
	uint32_t dst_conn_id
) {
	transport.send(RST(src_conn_id, dst_conn_id));
}

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::did_recv_RST(
	core::Buffer &&packet
) {
	// Bounds check
	if(packet.size() < 10) {
		return;
	}

	auto src_conn_id = packet.read_uint32_be_unsafe(6);
	auto dst_conn_id = packet.read_uint32_be_unsafe(2);
	if(src_conn_id == this->src_conn_id && dst_conn_id == this->dst_conn_id) {
		SPDLOG_ERROR(
			"Stream transport {{ Src: {}, Dst: {} }}: RST",
			src_addr.to_string(),
			dst_addr.to_string()
		);
		reset();
		close();
	}
}

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::send_DATA(
	core::Buffer &&packet
) {
	transport.send(std::move(packet));
}


/*!
	\li handles the packet fragments received on the connection.
	\li queues the packets if they are received out of order
	\li if/once all data before the current packet is received, its read and passed onto the delegate to be reassembled into meaningful message

	\param packet the bytes received
*/
template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::did_recv_DATA(
	core::Buffer &&packet
) {
	auto &p = *reinterpret_cast<StreamPacket *>(&packet);

	// Bounds check
	if(p.size() < 30 + crypto_aead_aes256gcm_ABYTES) {
		return;
	}

	auto src_conn_id = p.read_uint32_be_unsafe(6);
	auto dst_conn_id = p.read_uint32_be_unsafe(2);
	if(src_conn_id != this->src_conn_id || dst_conn_id != this->dst_conn_id) { // Wrong connection id, send RST
		SPDLOG_ERROR(
			"Stream transport {{ Src: {}, Dst: {} }}: DATA: Connection id mismatch: {}, {}, {}, {}",
			src_addr.to_string(),
			dst_addr.to_string(),
			src_conn_id,
			this->src_conn_id,
			dst_conn_id,
			this->dst_conn_id
		);
		send_RST(src_conn_id, dst_conn_id);
		return;
	}

	uint8_t nonce[12];
	for(int i = 0; i < 4; i++) {
		nonce[i] = rx_IV[i] ^ packet.data()[2+i];
	}
	for(int i = 0; i < 8; i++) {
		nonce[4+i] = rx_IV[4+i] ^ packet.data()[12+i];
	}
	auto res = crypto_aead_aes256gcm_decrypt_afternm(
		p.data() + 20,
		nullptr,
		nullptr,
		p.data() + 20,
		p.size() - 20,
		p.data() + 2,
		18,
		nonce,
		&rx_ctx
	);

	if(res < 0) {
		SPDLOG_ERROR(
			"Stream transport {{ Src: {}, Dst: {} }}: DATA: Decryption failure: {}, {}",
			src_addr.to_string(),
			dst_addr.to_string(),
			this->src_conn_id,
			this->dst_conn_id
		);
		send_RST(src_conn_id, dst_conn_id);
		return;
	}

	p.truncate_unsafe(crypto_aead_aes256gcm_ABYTES);

	SPDLOG_TRACE("DATA <<< {}: {}, {}", dst_addr.to_string(), p.offset(), p.length());

	if(conn_state == ConnectionState::DialRcvd) {
		conn_state = ConnectionState::Established;
	} else if(conn_state != ConnectionState::Established) {
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

	// Short circuit if stream is waiting to be flushed.
	if(stream.wait_flush) {
		return;
	}

	// Set stream size if fin bit set
	if(p.is_fin_set() && stream.state == RecvStream::State::Recv) {
		stream.size = offset + length;
		stream.state = RecvStream::State::SizeKnown;
	}

	// Cover header
	p.cover_unsafe(30);

	// Check if length matches packet
	// FIXME: Why even have length? Can just set from the packet
	if(p.size() != length) {
		return;
	}

	// Add to ack range
	ack_ranges.add_packet_number(packet_number);

	// Start ack delay timer if not already active
	if(!ack_timer_active) {
		ack_timer_active = true;
		ack_timer.template start<Self, &Self::ack_timer_cb>(25, 0);
	}

	// Short circuit on no new data
	if(offset + length <= stream.read_offset) {
		return;
	}

	// Check if data can be read immediately
	if(offset <= stream.read_offset) {
		// Cover bytes which have already been read
		p.cover_unsafe(stream.read_offset - offset);

		// Read bytes and update offset
		auto res = delegate->did_recv_bytes(*this, std::move(packet), stream.stream_id);
		if(res < 0) {
			return;
		}
		stream.read_offset = offset + length;

		// Read any out of order data
		auto iter = stream.recv_packets.begin();
		while(iter != stream.recv_packets.end()) {
			// Short circuit if data can't be read immediately
			if(iter->second.offset > stream.read_offset) {
				break;
			}

			auto offset = iter->second.offset;
			auto length = iter->second.length;

			// Check new data
			if(offset + length > stream.read_offset) {
				// Cover bytes which have already been read
				iter->second.packet.cover_unsafe(stream.read_offset - offset);

				// Read bytes and update offset
				SPDLOG_DEBUG("Out of order: {}, {}, {:spn}", offset, length, spdlog::to_hex(iter->second.packet.data(), iter->second.packet.data() + iter->second.packet.size()));
				auto res = delegate->did_recv_bytes(*this, std::move(iter->second).packet, stream.stream_id);
				if(res < 0) {
					return;
				}

				stream.read_offset = offset + length;
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
		SPDLOG_DEBUG("Queue for later: {}, {}, {:spn}", offset, length, spdlog::to_hex(packet.data(), packet.data() + packet.size()));
		stream.recv_packets.try_emplace(
			offset,
			asyncio::EventLoop::now(),
			offset,
			length,
			std::move(packet)
		);

		// Check all data received
		if (stream.check_finish()) {
			stream.state = RecvStream::State::AllRecv;
		}
	}
}

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::send_ACK() {
	int size = ack_ranges.ranges.size() > 171 ? 171 : ack_ranges.ranges.size();

	core::Buffer packet({0, 2}, 20+8*size);

	packet.write_uint32_be_unsafe(2, src_conn_id);
	packet.write_uint32_be_unsafe(6, dst_conn_id);
	packet.write_uint16_be_unsafe(10, size);
	packet.write_uint64_be_unsafe(12, ack_ranges.largest);

	int i = 20;
	auto iter = ack_ranges.ranges.begin();
	// Upto 85 gaps
	for(
		;
		i <= 12+size*8 && iter != ack_ranges.ranges.end();
		i += 8, iter++
	) {
		packet.write_uint64_be_unsafe(i, *iter);
	}

	transport.send(std::move(packet));
}

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::did_recv_ACK(
	core::Buffer &&packet
) {
	SPDLOG_TRACE("ACK <<< {}", dst_addr.to_string());

	auto &p = *reinterpret_cast<StreamPacket *>(&packet);

	// Bounds check on header
	if(p.size() < 20) {
		return;
	}

	if(conn_state != ConnectionState::Established) {
		return;
	}

	auto src_conn_id = p.read_uint32_be_unsafe(6);
	auto dst_conn_id = p.read_uint32_be_unsafe(2);
	if(src_conn_id != this->src_conn_id || dst_conn_id != this->dst_conn_id) { // Wrong connection id, send RST
		SPDLOG_ERROR(
			"Stream transport {{ Src: {}, Dst: {} }}: ACK: Connection id mismatch: {}, {}, {}, {}",
			src_addr.to_string(),
			dst_addr.to_string(),
			src_conn_id,
			this->src_conn_id,
			dst_conn_id,
			this->dst_conn_id
		);
		send_RST(src_conn_id, dst_conn_id);
		return;
	}

	auto now = asyncio::EventLoop::now();

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
	p.cover_unsafe(20);

	uint64_t high = largest;
	bool gap = false;
	bool is_app_limited = (bytes_in_flight < 0.8 * congestion_window);

	// Bounds check
	if(p.size() < size*8) {
		return;
	}

	for(
		uint16_t i = 0;
		i < size;
		i++, gap = !gap
	) {
		uint64_t range = p.read_uint64_be_unsafe(i*8);

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
					if(stream.acked_offset < iter->stream_offset + iter->data.size()) {
						// Still not fully acked, skip erase and abort
						break;
					}

					delegate->did_send_bytes(
						*this,
						std::move(iter->data)
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

	auto sent_iter = sent_packets.begin();

	// Determine lost packets
	while(sent_iter != sent_packets.end()) {
		// Condition for packet in flight to be considered lost
		// 1. more than 20 packets before largest acked - disabled for now
		// 2. more than 25ms before before largest acked
		if (/*sent_iter->first + 20 < largest_acked ||*/
			largest_sent_time > sent_iter->second.sent_time + 50) {
			SPDLOG_TRACE(
				"Stream transport {{ Src: {}, Dst: {} }}: Lost packet: {}, {}, {}",
				transport.src_addr.to_string(),
				transport.dst_addr.to_string(),
				sent_iter->first,
				largest_sent_time,
				sent_iter->second.sent_time
			);

			bytes_in_flight -= sent_iter->second.length;
			sent_iter->second.stream->bytes_in_flight -= sent_iter->second.length;
			lost_packets[sent_iter->first] = sent_iter->second;

			sent_iter++;
		} else {
			break;
		}
	}

	if(sent_iter == sent_packets.begin()) {
		// No lost packets, ignore
	} else {
		// Lost packets, congestion event
		auto last_iter = std::prev(sent_iter);
		auto &sent_packet = last_iter->second;

		if(sent_packet.sent_time > congestion_start) {
			// New congestion event
			SPDLOG_ERROR(
				"Stream transport {{ Src: {}, Dst: {} }}: Congestion event: {}, {}",
				transport.src_addr.to_string(),
				transport.dst_addr.to_string(),
				congestion_window,
				last_iter->first
			);
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
		sent_packets.erase(sent_packets.begin(), sent_iter);
	}

	// New packets
	send_pending_data();

	tlp_interval = DEFAULT_TLP_INTERVAL;
	tlp_timer.template start<Self, &Self::tlp_timer_cb>(tlp_interval, 0);
}

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::send_SKIPSTREAM(
	uint16_t stream_id,
	uint64_t offset
) {
	core::Buffer packet({0, 7}, 20);

	packet.write_uint32_be_unsafe(2, src_conn_id);
	packet.write_uint32_be_unsafe(6, dst_conn_id);
	packet.write_uint16_be_unsafe(10, stream_id);
	packet.write_uint64_be_unsafe(12, offset);

	transport.send(std::move(packet));
}

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::did_recv_SKIPSTREAM(
	core::Buffer &&packet
) {
	// Bounds check
	if(packet.size() < 20) {
		return;
	}

	auto &p = *reinterpret_cast<StreamPacket *>(&packet);

	SPDLOG_TRACE("SKIPSTREAM <<< {}: {}", dst_addr.to_string(), p.stream_id());

	auto src_conn_id = p.read_uint32_be_unsafe(6);
	auto dst_conn_id = p.read_uint32_be_unsafe(2);
	if(src_conn_id != this->src_conn_id || dst_conn_id != this->dst_conn_id) { // Wrong connection id, send RST
		SPDLOG_ERROR(
			"Stream transport {{ Src: {}, Dst: {} }}: SKIPSTREAM: Connection id mismatch: {}, {}, {}, {}",
			src_addr.to_string(),
			dst_addr.to_string(),
			src_conn_id,
			this->src_conn_id,
			dst_conn_id,
			this->dst_conn_id
		);
		send_RST(src_conn_id, dst_conn_id);
		return;
	}

	if(conn_state != ConnectionState::Established) {
		return;
	}

	auto stream_id = p.stream_id();
	auto offset = p.packet_number();
	auto &stream = get_or_create_send_stream(stream_id);

	// Check for duplicate
	if(offset < stream.acked_offset) {
		send_FLUSHSTREAM(stream_id, stream.acked_offset);
		return;
	}

	delegate->did_recv_skip_stream(*this, stream_id);

	flush_stream(stream_id);
}

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::send_FLUSHSTREAM(
	uint16_t stream_id,
	uint64_t offset
) {
	core::Buffer packet({0, 8}, 20);

	packet.write_uint32_be_unsafe(2, src_conn_id);
	packet.write_uint32_be_unsafe(6, dst_conn_id);
	packet.write_uint16_be_unsafe(10, stream_id);
	packet.write_uint64_be_unsafe(12, offset);

	transport.send(std::move(packet));
}

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::did_recv_FLUSHSTREAM(
	core::Buffer &&packet
) {
	// Bounds check
	if(packet.size() < 20) {
		return;
	}

	auto &p = *reinterpret_cast<StreamPacket *>(&packet);

	SPDLOG_TRACE("FLUSHSTREAM <<< {}: {}", dst_addr.to_string(), p.stream_id());

	auto src_conn_id = p.read_uint32_be_unsafe(6);
	auto dst_conn_id = p.read_uint32_be_unsafe(2);
	if(src_conn_id != this->src_conn_id || dst_conn_id != this->dst_conn_id) { // Wrong connection id, send RST
		SPDLOG_ERROR(
			"Stream transport {{ Src: {}, Dst: {} }}: FLUSHSTREAM: Connection id mismatch: {}, {}, {}, {}",
			src_addr.to_string(),
			dst_addr.to_string(),
			src_conn_id,
			this->src_conn_id,
			dst_conn_id,
			this->dst_conn_id
		);
		send_RST(src_conn_id, dst_conn_id);
		return;
	}

	if(conn_state != ConnectionState::Established) {
		return;
	}

	auto stream_id = p.stream_id();
	auto offset = p.packet_number();

	auto &stream = get_or_create_recv_stream(stream_id);
	auto old_offset = stream.read_offset;

	// Check for duplicate
	if(old_offset >= offset) {
		return;
	}

	stream.state_timer.stop();

	stream.recv_packets.clear();
	stream.read_offset = offset;
	stream.wait_flush = false;

	delegate->did_recv_flush_stream(*this, stream_id, offset, old_offset);

	send_FLUSHCONF(stream_id);
}

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::send_FLUSHCONF(
	uint16_t stream_id
) {
	core::Buffer packet({0, 9}, 12);

	packet.write_uint32_be_unsafe(2, src_conn_id);
	packet.write_uint32_be_unsafe(6, dst_conn_id);
	packet.write_uint16_be_unsafe(10, stream_id);

	transport.send(std::move(packet));
}

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::did_recv_FLUSHCONF(
	core::Buffer &&packet
) {
	// Bounds check
	if(packet.size() < 12) {
		return;
	}

	auto &p = *reinterpret_cast<StreamPacket *>(&packet);

	SPDLOG_TRACE("FLUSHCONF <<< {}: {}", dst_addr.to_string(), p.stream_id());

	auto src_conn_id = p.read_uint32_be_unsafe(6);
	auto dst_conn_id = p.read_uint32_be_unsafe(2);
	if(src_conn_id != this->src_conn_id || dst_conn_id != this->dst_conn_id) { // Wrong connection id, send RST
		SPDLOG_ERROR(
			"Stream transport {{ Src: {}, Dst: {} }}: FLUSHCONF: Connection id mismatch: {}, {}, {}, {}",
			src_addr.to_string(),
			dst_addr.to_string(),
			src_conn_id,
			this->src_conn_id,
			dst_conn_id,
			this->dst_conn_id
		);
		send_RST(src_conn_id, dst_conn_id);
		return;
	}

	if(conn_state != ConnectionState::Established) {
		return;
	}

	auto stream_id = p.stream_id();
	auto &stream = get_or_create_send_stream(stream_id);

	stream.state_timer.stop();
}

//---------------- Protocol functions end ----------------//


//---------------- Delegate functions begin ----------------//

//! Callback function when trying to establish a connection with a peer. Sends a DIAL packet to initiate the handshake
template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::did_dial(
	BaseTransport &
) {
	if(conn_state != ConnectionState::Listen) {
		return;
	}

	// Begin handshake
	dialled = true;

	state_timer_interval = 1000;
	state_timer.template start<Self, &Self::dial_timer_cb>(state_timer_interval, 0);

	src_conn_id = (uint32_t)std::random_device()();
	send_DIAL();
	conn_state = ConnectionState::DialSent;
}

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::did_close(
	BaseTransport &
) {
	delegate->did_close(*this);
	transport_manager.erase(dst_addr);
}

//! Receives the packet and processes them
/*!
	Determines the type of packet by reading the first byte and redirects the packet to appropriate function for further processing

	\li \b byte :	\b type
	\li 0		:	DATA
	\li 1		:	DATA + FIN
	\li 2		:	ACK
	\li 3		:	DIAL
	\li 4		:	DIALCONF
	\li 5		:	CONF
	\li 6		:	RST
*/
template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::did_recv_packet(
	BaseTransport &,
	core::Buffer &&packet
) {
	auto type = packet.read_uint8(1);
	if(type == std::nullopt || packet.read_uint8_unsafe(0) != 0) {
		return;
	}

	switch(type.value()) {
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
		// SKIPSTREAM
		case 7: did_recv_SKIPSTREAM(std::move(packet));
		break;
		// FLUSHSTREAM
		case 8: did_recv_FLUSHSTREAM(std::move(packet));
		break;
		// FLUSHCONF
		case 9: did_recv_FLUSHCONF(std::move(packet));
		break;
		// UNKNOWN
		default: SPDLOG_TRACE("UNKNOWN <<< {}", dst_addr.to_string());
		break;
	}
}

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::did_send_packet(
	BaseTransport &,
	core::Buffer &&packet
) {
	auto type = packet.read_uint8(1);
	if(type == std::nullopt) {
		return;
	}

	switch(type.value()) {
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
		// SKIPSTREAM
		case 7: SPDLOG_TRACE("SKIPSTREAM >>> {}", dst_addr.to_string());
		break;
		// FLUSHSTREAM
		case 8: SPDLOG_TRACE("FLUSHSTREAM >>> {}", dst_addr.to_string());
		break;
		// FLUSHCONF
		case 9: SPDLOG_TRACE("FLUSHCONF >>> {}", dst_addr.to_string());
		break;
		// UNKNOWN
		default: SPDLOG_TRACE("UNKNOWN >>> {}", dst_addr.to_string());
		break;
	}
}

//---------------- Delegate functions end ----------------//

template<typename DelegateType, template<typename> class DatagramTransport>
StreamTransport<DelegateType, DatagramTransport>::StreamTransport(
	core::SocketAddress const &src_addr,
	core::SocketAddress const &dst_addr,
	BaseTransport &transport,
	core::TransportManager<StreamTransport<DelegateType, DatagramTransport>> &transport_manager,
	uint8_t const* remote_static_pk
) : transport(transport),
	transport_manager(transport_manager),
	state_timer(this),
	pacing_timer(this),
	tlp_timer(this),
	ack_timer(this),
	src_addr(src_addr),
	dst_addr(dst_addr),
	delegate(nullptr) {
	if(sodium_init() == -1) {
		throw;
	}

	if (remote_static_pk != nullptr) {
		std::memcpy(this->remote_static_pk, remote_static_pk, crypto_box_PUBLICKEYBYTES);
	}
}


//! sets up the delegate when building an application over this transport. Also sets this class as the delegate of the underlying datagram_transport
/*!
	\param delegate a DelegateType pointer to the application class instance which uses this transport
*/
template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::setup(
	DelegateType *delegate,
	uint8_t const* static_sk
) {
	this->delegate = delegate;

	std::memcpy(this->static_sk, static_sk, crypto_box_SECRETKEYBYTES);
	crypto_scalarmult_base(this->static_pk, this->static_sk);

	crypto_kx_keypair(this->ephemeral_pk, this->ephemeral_sk);

	transport.setup(this);
}


//! called by higher level application to send data
/*!
	\li adds data to appropriate send stream and
	\li calls send_pending_data to schedule pacing_timer_cb

	\return an integer 0 if successful, negative otherwise
*/
template<typename DelegateType, template<typename> class DatagramTransport>
int StreamTransport<DelegateType, DatagramTransport>::send(
	core::Buffer &&bytes,
	uint16_t stream_id
) {
	if (conn_state != ConnectionState::Established) {
		return -2;
	}
	auto &stream = get_or_create_send_stream(stream_id);

	if(stream.state == SendStream::State::Ready) {
		stream.state = SendStream::State::Send;
	}

	// Abort if data queue is too big
	if(stream.queue_offset - stream.acked_offset > 20000000) {
		SPDLOG_ERROR("Data queue overflow");
		return -1;
	}

	auto size = bytes.size();

	// Add data to send queue
	stream.data_queue.emplace_back(
		std::move(bytes),
		stream.queue_offset
	);

	stream.queue_offset += size;

	// Handle idle stream
	if(stream.next_item_iterator == stream.data_queue.end()) {
		stream.next_item_iterator = std::prev(stream.data_queue.end());
	}

	// Handle idle connection
	if(sent_packets.size() == 0 && lost_packets.size() == 0 && send_queue.size() == 0) {
		tlp_timer.template start<Self, &Self::tlp_timer_cb>(tlp_interval, 0);
	}

	register_send_intent(stream);
	send_pending_data();

	return 0;
}

//! closes the connection and clears the entries
/*!
	\li resets all the connection data
	\li calls the close for underlying datagram transport
	\li notifies the delegate application about the close
	\li erases self entry from the transport manager which in turn destroys this instance
*/
template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::close() {
	reset();
	transport.close();
}

/*!
 * \return \b True if Stream connection state is Established. For other Conn. states returns \b False.
 */
template<typename DelegateType, template<typename> class DatagramTransport>
bool StreamTransport<DelegateType, DatagramTransport>::is_active() {
	if(conn_state == ConnectionState::Established) {
		return true;
	}

	return false;
}

/*!
 * \return adaptive rtt calculated for given Stream Transport
 */
template<typename DelegateType, template<typename> class DatagramTransport>
double StreamTransport<DelegateType, DatagramTransport>::get_rtt() {
	return rtt;
}

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::skip_timer_cb(RecvStream& stream) {
	if(stream.state_timer_interval >= 64000) { // Abort on too many retries
		stream.state_timer_interval = 0;
		SPDLOG_ERROR(
			"Stream transport {{ Src: {}, Dst: {} }}: Skip timeout: {}",
			this->src_addr.to_string(),
			this->dst_addr.to_string(),
			stream.stream_id
		);
		this->close();
		return;
	}

	uint64_t offset;

	if(stream.recv_packets.size() > 0) {
		auto &packet = stream.recv_packets.rbegin()->second;
		offset = packet.offset + packet.length;
	} else {
		offset = stream.read_offset;
	}

	this->send_SKIPSTREAM(stream.stream_id, offset);

	stream.state_timer_interval *= 2;
	stream.state_timer.template start<Self, RecvStream, &Self::skip_timer_cb>(stream.state_timer_interval, 0);
}

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::skip_stream(
	uint16_t stream_id
) {
	auto &stream = get_or_create_recv_stream(stream_id);
	stream.wait_flush = true;

	uint64_t offset;

	if(stream.recv_packets.size() > 0) {
		auto &packet = stream.recv_packets.rbegin()->second;
		offset = packet.offset + packet.length;
	} else {
		offset = stream.read_offset;
	}

	send_SKIPSTREAM(stream_id, offset);

	stream.state_timer_interval = 1000;
	stream.state_timer.template start<Self, RecvStream, &Self::skip_timer_cb>(stream.state_timer_interval, 0);
}

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::flush_timer_cb(SendStream& stream) {
	if(stream.state_timer_interval >= 64000) { // Abort on too many retries
		stream.state_timer_interval = 0;
		SPDLOG_ERROR(
			"Stream transport {{ Src: {}, Dst: {} }}: Flush timeout: {}",
			this->src_addr.to_string(),
			this->dst_addr.to_string(),
			stream.stream_id
		);
		this->close();
		return;
	}

	this->send_FLUSHSTREAM(stream.stream_id, stream.sent_offset);

	stream.state_timer_interval *= 2;
	stream.state_timer.template start<Self, SendStream, &Self::flush_timer_cb>(stream.state_timer_interval, 0);
}

template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::flush_stream(
	uint16_t stream_id
) {
	auto &stream = get_or_create_send_stream(stream_id);

	stream.data_queue.clear();
	stream.queue_offset = stream.sent_offset;
	stream.next_item_iterator = stream.data_queue.end();
	stream.bytes_in_flight = 0;
	stream.acked_offset = stream.sent_offset;
	stream.outstanding_acks.clear();

	// Remove previously sent packets
	auto sent_iter = sent_packets.cbegin();
	while(sent_iter != sent_packets.cend()) {
		if(sent_iter->second.stream->stream_id != stream.stream_id) {
			sent_iter++;
			continue;
		}

		bytes_in_flight -= sent_iter->second.length;
		sent_iter = sent_packets.erase(sent_iter);
	}

	// Remove lost packets
	auto lost_iter = lost_packets.cbegin();
	while(lost_iter != lost_packets.cend()) {
		if(lost_iter->second.stream->stream_id != stream.stream_id) {
			lost_iter++;
			continue;
		}

		bytes_in_flight -= lost_iter->second.length;
		lost_iter = lost_packets.erase(lost_iter);
	}

	send_FLUSHSTREAM(stream_id, stream.sent_offset);

	stream.state_timer_interval = 1000;
	stream.state_timer.template start<Self, SendStream, &Self::flush_timer_cb>(stream.state_timer_interval, 0);
}

template<typename DelegateType, template<typename> class DatagramTransport>
uint8_t const* StreamTransport<DelegateType, DatagramTransport>::get_static_pk() {
	return static_pk;
}

template<typename DelegateType, template<typename> class DatagramTransport>
uint8_t const* StreamTransport<DelegateType, DatagramTransport>::get_remote_static_pk() {
	return remote_static_pk;
}

} // namespace stream
} // namespace marlin

#endif // MARLIN_STREAM_STREAMTRANSPORT_HPP
