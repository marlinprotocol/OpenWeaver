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
		Done,
		NotHandled
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


template <typename DelegateType>
void NearTransport<DelegateType>::did_recv_bytes(
	BaseTransport&,
	core::Buffer &&bytes
) {
	// if(bytes_remaining > bytes.size()) { // Partial message
	// 	SPDLOG_INFO("Partial: {}, {}, {}", buf_size, bytes.size(), bytes_remaining);
	// 	std::memcpy(buf + buf_size, bytes.data(), bytes.size());
	// 	buf_size += bytes.size();
	// 	bytes_remaining -= bytes.size();
	// } else { // Full message
		SPDLOG_INFO("Full: {}, {}, {}, {}", buf_size, bytes.size(), bytes_remaining, state);
		SPDLOG_INFO(spdlog::to_hex(bytes.data(), bytes.data() + bytes.size()));
		// std::memcpy(buf + buf_size, bytes.data(), bytes_remaining);
		// buf_size += bytes_remaining;
		// bytes.cover_unsafe(bytes_remaining);
		// bytes_remaining = 0;

	if(state == State::Idle) {
		state = State::LengthWait;
	} else if(state == State::LengthWait) {
		for(int i = 3; i >= 0; i--) {
			bytes_remaining = bytes.data() [i] + (bytes_remaining << 8);
		}
		std::memcpy(buf + buf_size, bytes.data(), 4);
		buf_size += 4;
		bytes.cover_unsafe(4);
		state = State::EnumOptionWait;
	} else if(state == State::EnumOptionWait) {
		uint8_t enum_opt = bytes.data()[0];
		SPDLOG_INFO("{}", enum_opt);
		if(enum_opt == 16) {
			state = State::HandshakeReading;
		} else {
			state = State::NotHandled;
		}
		std::memcpy(buf + buf_size, bytes.data(), 1);
		bytes_remaining--;
		buf_size++;
		bytes.cover_unsafe(1);
	} else if(state == State::HandshakeReading) {
		std::memcpy(buf + buf_size, bytes.data(), bytes.size());
		buf_size += bytes.size();
		bytes_remaining -= bytes.size();
		bytes.cover_unsafe(bytes.size());
		if(bytes_remaining == 0) {
			state = State::Done;
		}
	} else if(state == State::NotHandled) {
		SPDLOG_INFO("Kuch to gadbad hai");
		return;
	}

	if(bytes.size() > 0) {
		did_recv_bytes(transport, std::move(bytes));
	} else {
		// delegate->did_recv_message(*this, std::move(bytes));
		if(state == State::Done) {
			state = State::Idle;
			reply_handshake();
			SPDLOG_INFO("smthsmth");
			SPDLOG_INFO(spdlog::to_hex(buf, buf + buf_size));
		}
	}
	// }
}

// template <typename DelegateType>
// void NearTransport<DelegateType>::did_recv_bytes(
// 	BaseTransport&,
// 	core::Buffer &&bytes
// ) {
// 	// SPDLOG_INFO(
// 	// 	"message aaya: {}",
// 	// 	spdlog::to_hex(bytes.data(), bytes.data() + bytes.size())

// 	// );
// 	SPDLOG_INFO("Got something");
// 	std::memcpy(buf + buf_size, bytes.data(), bytes.size());
// 	buf_size += bytes.size();

// 	if(state == State::Idle) {
// 		// remaining 4
// 		// handle LengthWait
// 		uint8_t *buf = bytes.data();
// 		bytes_remaining = buf[3];
// 		bytes_remaining = buf[2] + (bytes_remaining << 8);
// 		bytes_remaining = buf[1] + (bytes_remaining << 8);
// 		bytes_remaining = buf[0] + (bytes_remaining << 8);
// 		bytes_remaining += 4;
// 		// Partial handshake
// 		// bytes.cover_unsafe(4);
// 		state = State::EnumOptionWait;
// 	}

// 	// Idle -> LengthWait -> HandshakeWait ---- Have entire handshake
// 	if(state == State::EnumOptionWait) {
// 		uint8_t option = bytes.read_uint8_unsafe(4);
// 		if(option == 16) {
// 			state = State::HandshakeReading;
// 		} else {
// 			// Handle this case.
// 		}
// 	}
// 	SPDLOG_INFO("message wapas bhejing1");
// 	if(state == State::HandshakeReading) {
// 		bytes_remaining -= bytes.size();
// 		if(bytes_remaining == 0 and buf_size > 0) {
// 			reply_handshake();
// 			state = State::Idle;
// 			buf_size = 0;
// 		}
// 	}
// 	SPDLOG_INFO("message receive waala");
// 	delegate->did_recv_message(*this, std::move(bytes));
// 	SPDLOG_INFO("smth");
// }





template <typename DelegateType>
void NearTransport<DelegateType>::reply_handshake() {
	SPDLOG_INFO("Replying");
	uint8_t near_node_key[33], gateway_key[33];
	std::memcpy(near_node_key, buf + 13, 33);
	std::memcpy(gateway_key, buf + 46, 33);
	std::memcpy(buf + 13, gateway_key, 33);
	std::memcpy(buf + 46, near_node_key, 33);

	uint8_t near_node_signature[crypto_sign_BYTES];
	memcpy(near_node_signature, buf + buf_size - crypto_sign_BYTES, crypto_sign_BYTES); // copy last crypto_sign_BYTES

	uint8_t msg_sig[74];
	bool flag = true; // true if nearkey < gatewayKey
	for(int i = 0; i < 33; i++) {
		if(near_node_key[i] == gateway_key[i]) {
			// nothing
		} else {
			if(near_node_key[i] > gateway_key[i]) {
				flag = false;
			} else {
				flag = true;
			}
			break;
		}
	}
	if(flag) {
		memcpy(msg_sig, near_node_key, 33);
		memcpy(msg_sig + 33, gateway_key, 33);
	} else {
		memcpy(msg_sig, gateway_key, 33);
		memcpy(msg_sig + 33, near_node_key, 33);
	}
	memcpy(msg_sig + 66, buf + buf_size - 73, 8);
	SPDLOG_INFO(
		"Signature message: {}",
		spdlog::to_hex(msg_sig, msg_sig + 74)
	);
// Hash the message
	using namespace CryptoPP;
	SHA256 sha256;
	uint8_t hashed_message[32];
	sha256.Update(msg_sig, 74);
	sha256.TruncatedFinal(hashed_message, 32);

	if(crypto_sign_verify_detached(near_node_signature, hashed_message, 32, near_node_key + 1) != 0) {
		SPDLOG_INFO("Signature verification failed");
	} else {
		SPDLOG_INFO("Signature verified successfully");
	}

	uint8_t myPrivateKey[crypto_sign_SECRETKEYBYTES];
	memcpy(myPrivateKey, delegate->static_sk, crypto_sign_SECRETKEYBYTES);
		SPDLOG_INFO("{}", spdlog::to_hex(myPrivateKey, myPrivateKey + 64));
		SPDLOG_INFO("{}", spdlog::to_hex(hashed_message, hashed_message + 32));
	uint8_t mySignature[64];
	crypto_sign_detached(mySignature, NULL, hashed_message, 32, myPrivateKey);
		// if(crypto_sign_verify_detached(mySignature, hashed_message, 32, delegate->static_pk) != 0) {
		// 	SPDLOG_INFO("BAD");
		// } else {
		// 	SPDLOG_INFO("GOOD");
		// }
	memcpy(buf + buf_size - 64, mySignature, 64);

	core::Buffer message(buf, buf_size);
	SPDLOG_INFO(
		"Message replying to NearTransport: {}",
		spdlog::to_hex(message.data(), message.data() + message.size())
	);
	transport.send(std::move(message));
}

}
}

#endif