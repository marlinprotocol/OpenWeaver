#ifndef MARLIN_NEAR_NEARTRANSPORT_HPP
#define MARLIN_NEAR_NEARTRANSPORT_HPP

// #include <spdlog/spdlog.h>
#include <marlin/asyncio/tcp/TcpTransport.hpp>
#include <cryptopp/sha.h>
#include <sodium.h>

#include <cryptopp/files.h>
#include <cryptopp/filters.h>
#include <cryptopp/hmac.h>
#include <cryptopp/modarith.h>
#include <cryptopp/oids.h>



namespace marlin {
namespace near {

template <typename DelegateType>
class NearTransport {
private:
	using BaseTransport = asyncio::TcpTransport<NearTransport<DelegateType>>;
	BaseTransport &transport;

	enum struct State {
		Idle,
		LengthWait,
		MessageReading,
		Done
	};
	State state = State::Idle;

	uint8_t *buf; // The limit is yet to be set.
	uint32_t buf_size = 0;
	uint32_t bytes_remaining = 0;
public:
	DelegateType *delegate;
	void did_recv_bytes(BaseTransport &transport, core::Buffer &&bytes);
	void did_dial(BaseTransport &transport);
	void did_send_bytes(BaseTransport &transport, core::Buffer &&bytes);
	void did_close(BaseTransport &transport, uint16_t reason);
	void send(core::Buffer &&message);

	NearTransport(
		core::SocketAddress const &src_addr,
		core::SocketAddress const &dst_addr,
		BaseTransport &transport
	);

	void setup(DelegateType *delegate);
	void reply_handshake();
};

template<typename DelegateType>
void NearTransport<DelegateType>::did_dial(BaseTransport &) {

}

template<typename DelegateType>
void NearTransport<DelegateType>::did_send_bytes(BaseTransport &, core::Buffer &&) {

}

template<typename DelegateType>
void NearTransport<DelegateType>::did_close(BaseTransport &, uint16_t reason) {
	delegate->did_close(*this, reason);
}

template<typename DelegateType>
NearTransport<DelegateType>::NearTransport(
	core::SocketAddress const &,
	core::SocketAddress const &,
	BaseTransport &transport
): transport(transport) {
}

template<typename DelegateType>
void NearTransport<DelegateType>::setup(
	DelegateType *delegate
) {
	this->delegate = delegate;

	transport.setup(this);
}

template<typename DelegateType>
void NearTransport<DelegateType>::send(
	core::Buffer &&message
) {
	transport.send(std::move(message));
}


template <typename DelegateType>
void NearTransport<DelegateType>::did_recv_bytes(
	BaseTransport&,
	core::Buffer &&bytes
) {
	// SPDLOG_DEBUG("{}", spdlog::to_hex(bytes.data(), bytes.data() + bytes.size()));
	if(bytes_remaining > bytes.size()) { // Partial message
		SPDLOG_DEBUG("Partial: {}, {}, {}", buf_size, bytes.size(), bytes_remaining);
		std::memcpy(buf + buf_size, bytes.data(), bytes.size());
		buf_size += bytes.size();
		bytes_remaining -= bytes.size();
		bytes.cover_unsafe(bytes.size());
	} else { // Full message
		SPDLOG_DEBUG("Full: {}, {}, {}, {}", buf_size, bytes.size(), bytes_remaining, state);
		std::memcpy(buf + buf_size, bytes.data(), bytes_remaining);
		buf_size += bytes_remaining;
		bytes.cover_unsafe(bytes_remaining);
		bytes_remaining = 0;

		if(state == State::Idle) {
			bytes_remaining = 4;
			buf = new uint8_t[4];
			state = State::LengthWait;
		} else if(state == State::LengthWait) {
			bytes_remaining = 0;
			for(int i = 3; i >= 0; i--) {
				bytes_remaining = buf[i] + (bytes_remaining << 8);
			}
			uint8_t size[4];
			memcpy(size, buf, 4);
			buf = new uint8_t[bytes_remaining + 4];
			memcpy(buf, size, 4);
			state = State::MessageReading;
		} else if(state == State::MessageReading) {
			if(bytes_remaining == 0) {
				state = State::Done;
				core::Buffer message(buf, buf_size);
				message.cover_unsafe(4);
				delegate->did_recv_message(*this, std::move(message));
				state = State::Idle;
				bytes_remaining = 0;
				buf_size = 0;
			}
		}

		if(bytes.size() > 0) {
			did_recv_bytes(transport, std::move(bytes));
		}
	}
}

} // near
} // marlin

#endif // MARLIN_NEAR_NEARTRANSPORT_HPP