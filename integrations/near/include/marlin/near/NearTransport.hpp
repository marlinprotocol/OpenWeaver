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
	typedef asyncio::TcpTransport<NearTransport<DelegateType>> BaseTransport;
	BaseTransport &transport;

	enum struct State {
		Idle,
		LengthWait,
		EnumOptionWait,
		HandshakeReading,
		BlockReading,
		Done,
		NotHandled
	};
	State state = State::LengthWait;

	uint8_t *buf; // The limit is yet to be set.
	uint32_t buf_size = 0;
	uint32_t total_size = 0;
	uint32_t bytes_remaining = 4;
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
void NearTransport<DelegateType>::did_close(BaseTransport &, uint16_t) {

}

template<typename DelegateType>
NearTransport<DelegateType>::NearTransport(
	core::SocketAddress const &,
	core::SocketAddress const &,
	BaseTransport &transport
): transport(transport) {
	buf = new uint8_t[10000];
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
	// SPDLOG_INFO("{}", spdlog::to_hex(bytes.data(), bytes.data() + bytes.size()));
	if(bytes_remaining > bytes.size()) { // Partial message
		SPDLOG_INFO("Partial: {}, {}, {}", buf_size, bytes.size(), bytes_remaining);
		std::memcpy(buf + buf_size, bytes.data(), bytes.size());
		buf_size += bytes.size();
		bytes_remaining -= bytes.size();
		bytes.cover_unsafe(bytes.size());
	} else { // Full message
		SPDLOG_INFO("Full: {}, {}, {}, {}, {}", buf_size, bytes.size(), bytes_remaining, state, total_size);
		std::memcpy(buf + buf_size, bytes.data(), bytes_remaining);
		buf_size += bytes_remaining;
		bytes.cover_unsafe(bytes_remaining);
		bytes_remaining = 0;

		if(state == State::Idle) {
			bytes_remaining = 4;
			state = State::LengthWait;
		} else if(state == State::LengthWait) {
			for(int i = 3; i >= 0; i--) {
				total_size = buf[i] + (total_size << 8);
				SPDLOG_INFO("{}: {}", i, buf[i]);
			}
			total_size += 4;
			bytes_remaining = 1;
			state = State::EnumOptionWait;
		} else if(state == State::EnumOptionWait) {
			uint8_t enum_opt = buf[4];
			SPDLOG_INFO("{}", enum_opt);
			if(enum_opt == 0x10) {
				state = State::HandshakeReading;
			} else if(enum_opt == 0xb) {
				state = State::BlockReading;
			} else {
				state = State::NotHandled;
			}
			bytes_remaining = total_size - buf_size;
		} else if(state == State::HandshakeReading) {
			if(bytes_remaining == 0) {
				state = State::Done;
			}
		} else if(state == State::BlockReading) {
			if(bytes_remaining == 0) {
				state = State::Done;
			}
		} else if(state == State::NotHandled) {
			SPDLOG_INFO("Kuch to gadbad hai");
			if(bytes_remaining == 0) {
				state = State::Done;
			}
			// return;
		}

		if(bytes_remaining == 0) {
			uint8_t *cur_msg = new uint8_t[buf_size];
			std::memcpy(cur_msg, buf, buf_size);
			core::Buffer message(cur_msg, buf_size);
			delegate->did_recv_message(*this, std::move(message));
			state = State::LengthWait;
			bytes_remaining = 4;
			buf_size = total_size = 0;
		}
		if(bytes.size() > 0) {
			did_recv_bytes(transport, std::move(bytes));
		}
	}
}

}
}

#endif