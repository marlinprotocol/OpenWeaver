#ifndef MARLIN_ONRAMP_NEAR_HPP
#define MARLIN_ONRAMP_NEAR_HPP

#include <marlin/multicast/DefaultMulticastClient.hpp>
#include <marlin/near/NearTransportFactory.hpp>
#include <marlin/near/NearTransport.hpp>
#include <cryptopp/blake2.h>

using namespace marlin::near;
using namespace marlin::core;
using namespace marlin::multicast;

namespace marlin {
namespace near {

static const char b58digits_ordered[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

class OnRampNear {
public:
	DefaultMulticastClient<OnRampNear> multicastClient;
	uint8_t static_sk[crypto_sign_SECRETKEYBYTES], static_pk[crypto_sign_PUBLICKEYBYTES]; // should be moved to DefaultMulticastClient and add a function there to sign a message.
	NearTransport <OnRampNear> *nearTransport = nullptr;
	NearTransportFactory <OnRampNear, OnRampNear> f;

	void handle_handshake(core::Buffer &&message);
	void handle_transaction(core::Buffer &&message);
	void handle_block(core::Buffer &&message);

	OnRampNear(DefaultMulticastClientOptions clop): multicastClient(clop) {
		multicastClient.delegate = this;
		memcpy(static_sk, clop.static_sk, crypto_sign_SECRETKEYBYTES);
		memcpy(static_pk, clop.static_pk, crypto_sign_PUBLICKEYBYTES);
		if(sodium_init() == -1) {
			throw;
		}

		char *b58 = (char*)std::malloc(100);
		SPDLOG_INFO(
			"PrivateKey: {} \n PublicKey: {}",
			spdlog::to_hex(clop.static_sk, clop.static_sk + crypto_sign_SECRETKEYBYTES),
			spdlog::to_hex(clop.static_pk, clop.static_pk + 32)
		);
		if(b58enc_mine(b58, 44, clop.static_pk, 32)) {
			printf("%s\n", b58);
		} else {
			printf("Failed\n");
		}

		SPDLOG_INFO(
			"PrivateKey: {} \n PublicKey: {}",
			spdlog::to_hex(static_sk, static_sk + 32),
			spdlog::to_hex(static_pk, static_pk + 32)
		);

		f.bind(SocketAddress::loopback_ipv4(8000));
		f.listen(*this);
	}


	bool b58enc_mine(char *b58, size_t b58_size, const uint8_t *data, size_t data_size) {
		memset(b58, 0, b58_size);
		for(size_t i = 0; i < b58_size; i++) {
			b58[i] = 0;
		}
		b58[b58_size] = '\0';
		int high = b58_size - 1;
		for(int i = 0; i < int(data_size); i++) {
			int j = b58_size - 1, carry = data[i];
			while((j > high || carry > 0) && j >= 0) {
				carry += 256 * b58[j];
				b58[j] = carry % 58;
				carry /= 58;
				j--;
			}
			high = j;
		}
		for(int i = 0; i < int(b58_size); i++) {
			b58[i] = b58digits_ordered[int(b58[i])];
		}
		return true;
	}

	void did_recv_message(NearTransport <OnRampNear> &, Buffer &&message) {
		SPDLOG_INFO(
			"msgSize: {}; Message received from NearTransport: {}",
			message.size(),
			spdlog::to_hex(message.data(), message.data() + message.size())
		);
		if(message.data()[0] == 0x10) {
			handle_handshake(std::move(message));
		} else if(message.data()[0] == 0xc) {
			handle_transaction(std::move(message));
		} else if(message.data()[0] == 0xb) {
			handle_block(std::move(message));
		} else if(message.data()[0] == 0xd) {
			SPDLOG_INFO(
				"This is a RoutedMessage"
			);
		} else {
			SPDLOG_INFO(
				"Something else: {}",
				message.data()[0]
			);
		}
	}

	template<typename T> // TODO: Code smell, remove later
	void did_recv_message(
		DefaultMulticastClient<OnRampNear> &,
		Buffer &&,
		T,
		uint16_t,
		uint64_t
	) {}

	void did_send_message(NearTransport<OnRampNear> &, Buffer &&message) {
		SPDLOG_INFO(
			"Transport {{ Src: {}, Dst: {} }}: Did send message: {} bytes",
			// transport.src_addr.to_string(),
			// transport.dst_addr.to_string(),
			message.size()
		);
	}

	void did_dial(NearTransport<OnRampNear> &) {
		// (void)transport;
		// transport.send(Buffer(new char[10], 10));
	}

	bool should_accept(SocketAddress const &) {
		return true;
	}


	void did_subscribe(
		DefaultMulticastClient<OnRampNear> &,
		uint16_t
	) {}

	void did_unsubscribe(
		DefaultMulticastClient<OnRampNear> &,
		uint16_t
	) {}

	void did_close(NearTransport<OnRampNear> &, uint16_t) {}

	void did_create_transport(NearTransport <OnRampNear> &transport) {
		nearTransport = &transport;
		transport.setup(this);
	}
};


void OnRampNear::handle_transaction(core::Buffer &&message) {
	SPDLOG_INFO("Handling transaction: {}", spdlog::to_hex(message.data(), message.data() + message.size()));

	CryptoPP::BLAKE2b blake2b((uint)8);
	blake2b.Update((uint8_t *)message.data(), message.size());
	uint64_t message_id;
	blake2b.TruncatedFinal((uint8_t *)&message_id, 8);
	multicastClient.ps.send_message_on_channel(
		1,
		message_id,
		message.data(),
		message.size()
	);
}

void OnRampNear::handle_block(core::Buffer &&message) {
	SPDLOG_INFO("Handling block");

	CryptoPP::BLAKE2b blake2b((uint)8);
	blake2b.Update((uint8_t *)message.data(), message.size());
	uint64_t message_id;
	blake2b.TruncatedFinal((uint8_t *)&message_id, 8);
	multicastClient.ps.send_message_on_channel(
		0,
		message_id,
		message.data(),
		message.size()
	);
}

void OnRampNear::handle_handshake(core::Buffer &&message) {
	SPDLOG_INFO("Replying");
	uint8_t *buf = message.data();
	uint32_t buf_size = message.size();
	std::swap_ranges(buf + 9, buf + 42, buf + 42);

	uint8_t near_key_offset = 42, gateway_key_offset = 9;

	uint8_t *near_node_signature = buf + buf_size - crypto_sign_BYTES;
	// memcpy(near_node_signature, buf + buf_size - crypto_sign_BYTES, crypto_sign_BYTES); // copy last crypto_sign_BYTES

	uint8_t msg_sig[74];
	bool flag = true; // true if nearkey < gatewayKey
	for(int i = 0; i < 33; i++) {
		if(buf[i + near_key_offset] == buf[i + gateway_key_offset]) {
			// nothing
		} else {
			if(buf[i + near_key_offset] > buf[i + gateway_key_offset]) {
				flag = false;
			} else {
				flag = true;
			}
			break;
		}
	}
	if(flag) {
		memcpy(msg_sig, buf + near_key_offset, 33);
		memcpy(msg_sig + 33, buf + gateway_key_offset, 33);
	} else {
		memcpy(msg_sig, buf + gateway_key_offset, 33);
		memcpy(msg_sig + 33, buf + near_key_offset, 33);
	}
	memcpy(msg_sig + 66, buf + buf_size - 73, 8);
	SPDLOG_INFO(
		"Signature buf: {}",
		spdlog::to_hex(msg_sig, msg_sig + 74)
	);
// Hash the buf
	using namespace CryptoPP;
	SHA256 sha256;
	uint8_t hashed_message[32];
	sha256.Update(msg_sig, 74);
	sha256.TruncatedFinal(hashed_message, 32);

	if(crypto_sign_verify_detached(near_node_signature, hashed_message, 32, buf + near_key_offset + 1) != 0) {
		SPDLOG_INFO("Signature verification failed");
	} else {
		SPDLOG_INFO("Signature verified successfully");
	}

		// SPDLOG_INFO("{}", spdlog::to_hex(myPrivateKey, myPrivateKey + 64));
		// SPDLOG_INFO("{}", spdlog::to_hex(hashed_message, hashed_message + 32));
	uint8_t *mySignature = buf + buf_size - 64;
	crypto_sign_detached(mySignature, NULL, hashed_message, 32, static_sk);
		if(crypto_sign_verify_detached(mySignature, hashed_message, 32, static_pk) != 0) {
			SPDLOG_INFO("BAD");
		} else {
			SPDLOG_INFO("GOOD");
		}
	memcpy(buf + buf_size - 64, mySignature, 64);
	message.uncover_unsafe(4);
	nearTransport->send(std::move(message));
}


}
}
#endif