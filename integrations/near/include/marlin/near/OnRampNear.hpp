#ifndef MARLIN_ONRAMP_NEAR_HPP
#define MARLIN_ONRAMP_NEAR_HPP

#include <marlin/near/NearTransportFactory.hpp>
#include <marlin/near/NearTransport.hpp>

using namespace marlin::near;
using namespace marlin::core;

namespace marlin {
namespace near {

static const char b58digits_ordered[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

class OnRampNear {
public:

	uint8_t static_sk[crypto_sign_SECRETKEYBYTES], static_pk[crypto_sign_PUBLICKEYBYTES];
	NearTransport <OnRampNear> *nearTransport = nullptr;
	NearTransportFactory <OnRampNear, OnRampNear> f;

	void reply_handshake(core::Buffer &&message);

	OnRampNear() {
		if(sodium_init() == -1) {
			throw;
		}

		crypto_sign_keypair(static_pk, static_sk);
		size_t *afterSize = (size_t*)malloc(10);
		*afterSize = (size_t)100;
		char *b58 = (char*)std::malloc(100);
		SPDLOG_INFO(
			"PrivateKey: {} \n PublicKey: {}",
			spdlog::to_hex(static_sk, static_sk + 32),
			spdlog::to_hex(static_pk, static_pk + 32)
		);
		if(b58enc(b58, afterSize, static_pk, 32)) {
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

	bool b58enc(char *b58, size_t *b58sz, const void *data, size_t binsz) {
		const uint8_t *bin = (uint8_t*)data;
		int carry;
		size_t i, j, high, zcount = 0;
		size_t size;

		while (zcount < binsz && !bin[zcount])
			++zcount;

		size = (binsz - zcount) * 138 / 100 + 1;
		uint8_t *message = new uint8_t[size];
		memset(message, 0, size);
		for (i = zcount, high = size - 1; i < binsz; ++i, high = j)
		{
			for (carry = bin[i], j = size - 1; (j > high) || carry; --j)
			{
				carry += 256 * message[j];
				message[j] = carry % 58;
				carry /= 58;
				if (!j) {
					// Otherwise j wraps to maxint which is > high
					break;
				}
			}
		}
		for (j = 0; j < size && !message[j]; ++j);

		if (*b58sz <= zcount + size - j)
		{
			std::cout << "Yp\n";
			*b58sz = zcount + size - j + 1;
			return false;
		}

		if (zcount)
			memset(b58, '1', zcount);
		for (i = zcount; j < size; ++i, ++j)
			b58[i] = b58digits_ordered[message[j]];
		b58[i] = '\0';
		*b58sz = i + 1;

		return true;
	}

	void did_recv_message(NearTransport <OnRampNear> &, Buffer &&message) {
		SPDLOG_INFO(
			"msgSize: {}; Message received from NearTransport: {}",
			message.size(),
			spdlog::to_hex(message.data(), message.data() + message.size())
		);
		if(message.data()[4] == 0x10) {
			reply_handshake(std::move(message));
		}
	}

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

	void did_close(NearTransport<OnRampNear> &, uint16_t) {}

	void did_create_transport(NearTransport <OnRampNear> &transport) {
		nearTransport = &transport;
		transport.setup(this);
	}
};


void OnRampNear::reply_handshake(core::Buffer &&message) {
	SPDLOG_INFO("Replying");
	uint8_t *buf = message.data();
	uint32_t buf_size = message.size();
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
		"Signature buf: {}",
		spdlog::to_hex(msg_sig, msg_sig + 74)
	);
// Hash the buf
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

		// SPDLOG_INFO("{}", spdlog::to_hex(myPrivateKey, myPrivateKey + 64));
		// SPDLOG_INFO("{}", spdlog::to_hex(hashed_message, hashed_message + 32));
	uint8_t mySignature[64];
	crypto_sign_detached(mySignature, NULL, hashed_message, 32, static_sk);
		// if(crypto_sign_verify_detached(mySignature, hashed_message, 32, delegate->static_pk) != 0) {
		// 	SPDLOG_INFO("BAD");
		// } else {
		// 	SPDLOG_INFO("GOOD");
		// }
	memcpy(buf + buf_size - 64, mySignature, 64);

	nearTransport->send(std::move(message));
}


}
}
#endif