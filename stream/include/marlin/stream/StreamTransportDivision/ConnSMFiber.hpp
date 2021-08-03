#include <marlin/core/Buffer.hpp>
#include <spdlog/spdlog.h>
#include <marlin/core/messages/BaseMessage.hpp>
#include "../Messages.hpp"
#include <sodium.h>

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
		ext_fabric(std::get<0>(init_tuple)) {}

	int did_recv(BaseMessage &&buf);
	void did_recv_DATA(DATA &&packet);

};

// Impl

// template <typename ExtFabric>
// void ConnSMFiber<ExtFabric>::did_recv_DATA(DATA &&packet) {
// 	if(!packet.validate(12 + crypto_aead_aes256gcm_ABYTES)) {
// 		return;
// 	}

// 	auto src_conn_id = packet.src_conn_id();
// 	auto dst_conn_id = packet.dst_conn_id();
// 	if(src_conn_id != this->src_conn_id || dst_conn_id != this->dst_conn_id) { // Wrong connection id, send RST
// 		SPDLOG_DEBUG(
// 			"Stream transport {{ Src: {}, Dst: {} }}: DATA: Connection id mismatch: {}, {}, {}, {}",
// 			src_addr.to_string(),
// 			dst_addr.to_string(),
// 			src_conn_id,
// 			this->src_conn_id,
// 			dst_conn_id,
// 			this->dst_conn_id
// 		);
// 		send_RST(src_conn_id, dst_conn_id);
// 		return;
// 	}

// 	if constexpr (is_encrypted) {
// 		auto res = crypto_aead_aes256gcm_decrypt_afternm(
// 			packet.payload() - 12,
// 			nullptr,
// 			nullptr,
// 			packet.payload() - 12,
// 			packet.payload_buffer().size(),
// 			packet.payload() - 28,
// 			16,
// 			packet.payload() + packet.payload_buffer().size() - 12,
// 			&rx_ctx
// 		);

// 		if(res < 0) {
// 			SPDLOG_DEBUG(
// 				"Stream transport {{ Src: {}, Dst: {} }}: DATA: Decryption failure: {}, {}",
// 				src_addr.to_string(),
// 				dst_addr.to_string(),
// 				this->src_conn_id,
// 				this->dst_conn_id
// 			);
// 			send_RST(src_conn_id, dst_conn_id);
// 			return;
// 		}
// 	}

// 	SPDLOG_TRACE("DATA <<< {}: {}, {}", dst_addr.to_string(), packet.offset(), packet.length());

// 	if(conn_state == ConnectionState::DialRcvd) {
// 		conn_state = ConnectionState::Established;

// 		if(dialled) {
// 			delegate->did_dial(*this);
// 		}
// 	} else if(conn_state != ConnectionState::Established) {
// 		return;
// 	}

// 	auto offset = packet.offset();
// 	auto length = packet.length();
// 	auto packet_number = packet.packet_number();

// 	auto &stream = get_or_create_recv_stream(packet.stream_id());

// 	// Short circuit once stream has been received fully.
// 	if(stream.state == RecvStream::State::AllRecv ||
// 		stream.state == RecvStream::State::Read) {
// 		return;
// 	}

// 	// Short circuit if stream is waiting to be flushed.
// 	if(stream.wait_flush) {
// 		return;
// 	}

// 	// Set stream size if fin bit set
// 	if(packet.is_fin_set() && stream.state == RecvStream::State::Recv) {
// 		stream.size = offset + length;
// 		stream.state = RecvStream::State::SizeKnown;
// 	}

// 	auto p = std::move(packet).payload_buffer();
// 	p.truncate_unsafe(crypto_aead_aes256gcm_ABYTES + 12);

// 	// Check if length matches packet
// 	// FIXME: Why even have length? Can just set from the packet
// 	if(p.size() != length) {
// 		return;
// 	}

// 	// Add to ack range
// 	ack_ranges.add_packet_number(packet_number);

// 	// Start ack delay timer if not already active
// 	if(!ack_timer_active) {
// 		ack_timer_active = true;
// 		ack_timer.template start<Self, &Self::ack_timer_cb>(25, 0);
// 	}

// 	// Short circuit on no new data
// 	if(offset + length <= stream.read_offset) {
// 		return;
// 	}

// 	// Check if data can be read immediately
// 	if(offset <= stream.read_offset) {
// 		// Cover bytes which have already been read
// 		p.cover_unsafe(stream.read_offset - offset);

// 		// Read bytes and update offset
// 		auto res = delegate->did_recv(*this, std::move(p), stream.stream_id);
// 		if(res < 0) {
// 			return;
// 		}
// 		stream.read_offset = offset + length;

// 		// Read any out of order data
// 		auto iter = stream.recv_packets.begin();
// 		while(iter != stream.recv_packets.end()) {
// 			// Short circuit if data can't be read immediately
// 			if(iter->second.offset > stream.read_offset) {
// 				break;
// 			}

// 			auto offset = iter->second.offset;
// 			auto length = iter->second.length;

// 			// Check new data
// 			if(offset + length > stream.read_offset) {
// 				// Cover bytes which have already been read
// 				iter->second.packet.cover_unsafe(stream.read_offset - offset);

// 				// Read bytes and update offset
// 				SPDLOG_DEBUG("Out of order: {}, {}, {:spn}", offset, length, spdlog::to_hex(iter->second.packet.data(), iter->second.packet.data() + iter->second.packet.size()));
// 				auto res = delegate->did_recv(*this, std::move(iter->second).packet, stream.stream_id);
// 				if(res < 0) {
// 					return;
// 				}

// 				stream.read_offset = offset + length;
// 			}

// 			// Next iter
// 			iter = stream.recv_packets.erase(iter);
// 		}

// 		// Check all data read
// 		if(stream.check_read()) {
// 			stream.state = RecvStream::State::Read;
// 			recv_streams.erase(stream.stream_id);
// 		}
// 	} else {
// 		// Queue packet for later processing
// 		SPDLOG_DEBUG("Queue for later: {}, {}, {:spn}", offset, length, spdlog::to_hex(p.data(), p.data() + p.size()));
// 		stream.recv_packets.try_emplace(
// 			offset,
// 			asyncio::EventLoop::now(),
// 			offset,
// 			length,
// 			std::move(p)
// 		);

// 		// Check all data received
// 		if (stream.check_finish()) {
// 			stream.state = RecvStream::State::AllRecv;
// 		}
// 	}
// }

// template<typename ExtFabric>
// void ConnSMFiber<ExtFabric>::send_RST(
// 	uint32_t src_conn_id,
// 	uint32_t dst_conn_id
// ) {
// 	ext_fabric.send(
// 		RST()
// 		.set_src_conn_id(src_conn_id)
// 		.set_dst_conn_id(dst_conn_id)
// 	);
// }

template <typename ExtFabric>
int ConnSMFiber<ExtFabric>::did_recv(BaseMessage &&) {
	return 0;
}

// template <typename ExtFabric>
// int ConnSMFiber<ExtFabric>::did_recv(BaseMessage &&) {
// 	SPDLOG_INFO(
// 		"Received in ConnSMFiber: {}", 
// 		buf.size()
// 	);
// 	auto type = buf.payload_buffer().read_uint8(1);
// 	if(type == std::nullopt || buf.payload_buffer().read_uint8_unsafe(0) != 0) {
// 		return -1;
// 	}

// 	switch(type.value()) {
// 		// DATA
// 		case 0:
// 		// DATA + FIN
// 		case 1: did_recv_DATA(std::move(buf));
// 		break;
// 		// ACK
// 		// case 2: did_recv_ACK(std::move(buf));
// 		// break;
// 		// // DIAL
// 		// case 3: did_recv_DIAL(std::move(buf));
// 		// break;
// 		// // DIALCONF
// 		// case 4: did_recv_DIALCONF(std::move(buf));
// 		// break;
// 		// // CONF
// 		// case 5: did_recv_CONF(std::move(buf));
// 		// break;
// 		// // RST
// 		// case 6: did_recv_RST(std::move(buf));
// 		// break;
// 		// // SKIPSTREAM
// 		// case 7: did_recv_SKIPSTREAM(std::move(buf));
// 		// break;
// 		// // FLUSHSTREAM
// 		// case 8: did_recv_FLUSHSTREAM(std::move(buf));
// 		// break;
// 		// // FLUSHCONF
// 		// case 9: did_recv_FLUSHCONF(std::move(buf));
// 		// break;
// 		// // UNKNOWN
// 		default: SPDLOG_TRACE("UNKNOWN <<< {}", dst_addr.to_string());
// 		break;
// 	}

// 	return 0;
// }











}
}