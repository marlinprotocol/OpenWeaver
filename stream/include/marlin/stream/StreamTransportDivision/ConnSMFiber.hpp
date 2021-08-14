#ifndef MARLIN_ASYNCIO_UDP_CONNSMFIBER_HPP
#define MARLIN_ASYNCIO_UDP_CONNSMFIBER_HPP

#include <marlin/core/Buffer.hpp>
#include <spdlog/spdlog.h>
#include <marlin/core/messages/BaseMessage.hpp>
#include "../Messages.hpp"
#include <sodium.h>
#include "../protocol/SendStream.hpp"
#include "../protocol/RecvStream.hpp"
#include "../protocol/AckRanges.hpp"


using namespace marlin::core;

namespace marlin {
namespace stream {

template <typename ExtFabric>
class ConnSMFiber {
private:
	enum struct ConnectionState {
		Listen,
		DialSent,
		DialRcvd,
		Established,
		Closing
	} conn_state = ConnectionState::Listen;

	using DATA = DATAWrapper<BaseMessage>;

	ExtFabric ext_fabric;
	SwitchFiberTemplate switchFiber;

	uint32_t src_conn_id = 0;
	uint32_t dst_conn_id = 0;

public:
	using OuterMessageType = Buffer;
	using InnerMessageType = Buffer;
 
	ConnSMFiber() {} 

	// template <typename Args>
	// ConnSMFiber(Args&&...) {}

	template<typename ExtTupleType>
	ConnSMFiber(std::tuple<ExtTupleType>&& init_tuple) :
		ext_fabric(std::get<0>(init_tuple)),
		switchFiber(std::get<1>(init_tuple)) {}

	int did_recv(BaseMessage &&buf);
	void did_recv_DATA(DATA &&packet);

};

// Impl

template <typename ExtFabric>
void ConnSMFiber<ExtFabric>::did_recv_DATA(DATA &&packet) {
	if(!packet.validate(12 + crypto_aead_aes256gcm_ABYTES)) {
		return;
	}

	auto src_conn_id = packet.src_conn_id();
	auto dst_conn_id = packet.dst_conn_id();
	if(src_conn_id != this->src_conn_id || dst_conn_id != this->dst_conn_id) { // Wrong connection id, send RST
		SPDLOG_DEBUG(
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

	if constexpr (is_encrypted) {
		auto res = crypto_aead_aes256gcm_decrypt_afternm(
			packet.payload() - 12,
			nullptr,
			nullptr,
			packet.payload() - 12,
			packet.payload_buffer().size(),
			packet.payload() - 28,
			16,
			packet.payload() + packet.payload_buffer().size() - 12,
			&rx_ctx
		);

		if(res < 0) {
			SPDLOG_DEBUG(
				"Stream transport {{ Src: {}, Dst: {} }}: DATA: Decryption failure: {}, {}",
				src_addr.to_string(),
				dst_addr.to_string(),
				this->src_conn_id,
				this->dst_conn_id
			);
			send_RST(src_conn_id, dst_conn_id);
			return;
		}
	}

	SPDLOG_TRACE("DATA <<< {}: {}, {}", dst_addr.to_string(), packet.offset(), packet.length());

	if(conn_state == ConnectionState::DialRcvd) {
		conn_state = ConnectionState::Established;

		if(dialled) {
			delegate->did_dial(*this);
		}
	} else if(conn_state != ConnectionState::Established) {
		return;
	}

	auto offset = packet.offset();
	auto length = packet.length();
	auto packet_number = packet.packet_number();

	auto &stream = get_or_create_recv_stream(packet.stream_id());

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
	if(packet.is_fin_set() && stream.state == RecvStream::State::Recv) {
		stream.size = offset + length;
		stream.state = RecvStream::State::SizeKnown;
	}

	auto p = std::move(packet).payload_buffer();
	p.truncate_unsafe(crypto_aead_aes256gcm_ABYTES + 12);

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
		auto res = delegate->did_recv(*this, std::move(p), stream.stream_id);
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
				auto res = delegate->did_recv(*this, std::move(iter->second).packet, stream.stream_id);
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
		SPDLOG_DEBUG("Queue for later: {}, {}, {:spn}", offset, length, spdlog::to_hex(p.data(), p.data() + p.size()));
		stream.recv_packets.try_emplace(
			offset,
			asyncio::EventLoop::now(),
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

template<typename ExtFabric>
void ConnSMFiber<ExtFabric>::send_RST(
	uint32_t src_conn_id,
	uint32_t dst_conn_id
) {
	ext_fabric.send(
		RST()
		.set_src_conn_id(src_conn_id)
		.set_dst_conn_id(dst_conn_id)
	);
}

template <typename ExtFabric, typename SwitchFiberTemplate>
int ConnSMFiber<ExtFabric, SwitchFiberTemplate>::did_recv(BaseMessage &&) {
	return 0;
}

template <typename ExtFabric>
int ConnSMFiber<ExtFabric>::did_recv(BaseMessage &&) {
	SPDLOG_INFO(
		"Received in ConnSMFiber: {}", 
		buf.size()
	);
	auto type = buf.payload_buffer().read_uint8(1);
	if(type == std::nullopt || buf.payload_buffer().read_uint8_unsafe(0) != 0) {
		return -1;
	}

	switch(type.value()) {
		// DATA
		case 0:
		// DATA + FIN
		case 1: did_recv_DATA(std::move(buf));
		break;
		// ACK
		case 2: did_recv_ACK(std::move(buf));
		break;
		// DIAL
		case 3: did_recv_DIAL(std::move(buf));
		break;
		// DIALCONF
		case 4: did_recv_DIALCONF(std::move(buf));
		break;
		// CONF
		case 5: did_recv_CONF(std::move(buf));
		break;
		// RST
		case 6: did_recv_RST(std::move(buf));
		break;
		// // SKIPSTREAM
		// case 7: did_recv_SKIPSTREAM(std::move(buf));
		// break;
		// // FLUSHSTREAM
		// case 8: did_recv_FLUSHSTREAM(std::move(buf));
		// break;
		// // FLUSHCONF
		// case 9: did_recv_FLUSHCONF(std::move(buf));
		// break;
		// // UNKNOWN
		default: SPDLOG_TRACE("UNKNOWN <<< {}", dst_addr.to_string());
		break;
	}

	return 0;
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

	switch(conn_state) {
	case ConnectionState::Listen: {
		if(packet.src_conn_id() != 0) { // Should have empty source
			SPDLOG_DEBUG(
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
			SPDLOG_DEBUG(
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
			SPDLOG_DEBUG(
				"Stream transport {{ Src: {}, Dst: {} }}: DIAL: Key derivation failure",
				src_addr.to_string(),
				dst_addr.to_string()
			);
			return;
		}

		randombytes_buf(nonce, crypto_aead_aes256gcm_NPUBBYTES);
		crypto_aead_aes256gcm_beforenm(&rx_ctx, rx);
		crypto_aead_aes256gcm_beforenm(&tx_ctx, tx);

		this->dst_conn_id = packet.dst_conn_id();
		this->src_conn_id = (uint32_t)std::random_device()();

		send_DIALCONF();

		conn_state = ConnectionState::DialRcvd;

		break;
	}

	case ConnectionState::DialSent: {
		if(packet.src_conn_id() != 0) { // Should have empty source
			SPDLOG_DEBUG(
				"Stream transport {{ Src: {}, Dst: {} }}: DIAL: Should have empty src: {}",
				src_addr.to_string(),
				dst_addr.to_string(),
				packet.src_conn_id()
			);
			return;
		}

		uint8_t pt[pt_len];
		auto res = crypto_box_seal_open(pt, packet.payload() + 10, ct_len, static_pk, static_sk);
		if (res < 0) {
			SPDLOG_DEBUG(
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
			SPDLOG_DEBUG(
				"Stream transport {{ Src: {}, Dst: {} }}: DIAL: Key derivation failure",
				src_addr.to_string(),
				dst_addr.to_string()
			);
			return;
		}

		randombytes_buf(nonce, crypto_aead_aes256gcm_NPUBBYTES);
		crypto_aead_aes256gcm_beforenm(&rx_ctx, rx);
		crypto_aead_aes256gcm_beforenm(&tx_ctx, tx);

		this->dst_conn_id = packet.dst_conn_id();

		state_timer.stop();
		state_timer_interval = 0;

		send_DIALCONF();

		conn_state = ConnectionState::DialRcvd;

		break;
	}

	case ConnectionState::DialRcvd:
	case ConnectionState::Established: {
		// Send existing ids
		// Wait for dst to RST and try to establish again
		// if this connection is stale
		send_DIALCONF();

		break;
	}

	case ConnectionState::Closing: {
		// Ignore
		break;
	}
	}
}


template<typename DelegateType, template<typename> class DatagramTransport>
void StreamTransport<DelegateType, DatagramTransport>::did_recv_ACK(
	ACK &&packet
) {
	if(!packet.validate()) {
		return;
	}

	if(conn_state != ConnectionState::Established) {
		return;
	}

	auto src_conn_id = packet.src_conn_id();
	auto dst_conn_id = packet.dst_conn_id();
	if(src_conn_id != this->src_conn_id || dst_conn_id != this->dst_conn_id) { // Wrong connection id, send RST
		SPDLOG_DEBUG(
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

	uint64_t largest = packet.packet_number();

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

	uint64_t high = largest;
	bool gap = false;
	bool is_app_limited = (bytes_in_flight < 0.8 * congestion_window);

	for(
		auto iter = packet.ranges_begin();
		iter != packet.ranges_end();
		++iter, gap = !gap
	) {
		uint64_t range = *iter;

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

				bool fully_acked = true;
				// Cleanup acked data items
				for(
					auto iter = stream.data_queue.begin();
					iter != stream.data_queue.end();
					iter = stream.data_queue.erase(iter)
				) {
					if(stream.acked_offset < iter->stream_offset + iter->data.size()) {
						// Still not fully acked, skip erase and abort
						fully_acked = false;
						break;
					}

					delegate->did_send(
						*this,
						std::move(iter->data)
					);
				}

				if(fully_acked) {
					stream.next_item_iterator = stream.data_queue.end();
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
			// SPDLOG_TRACE(
			// 	"Stream transport {{ Src: {}, Dst: {} }}: Lost packet: {}, {}, {}",
			// 	transport.src_addr.to_string(),	// SWITCH
			// 	transport.dst_addr.to_string(), // SWITCH
			// 	sent_iter->first,
			// 	largest_sent_time,
			// 	sent_iter->second.sent_time
			// );

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
			SPDLOG_DEBUG(
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








}
}

#endif