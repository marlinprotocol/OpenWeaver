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
		uint8_t *buf = new uint8_t[size];
		memset(buf, 0, size);
		for (i = zcount, high = size - 1; i < binsz; ++i, high = j)
		{
			for (carry = bin[i], j = size - 1; (j > high) || carry; --j)
			{
				carry += 256 * buf[j];
				buf[j] = carry % 58;
				carry /= 58;
				if (!j) {
					// Otherwise j wraps to maxint which is > high
					break;
				}
			}
		}
		for (j = 0; j < size && !buf[j]; ++j);

		if (*b58sz <= zcount + size - j)
		{
			std::cout << "Yp\n";
			*b58sz = zcount + size - j + 1;
			return false;
		}

		if (zcount)
			memset(b58, '1', zcount);
		for (i = zcount; j < size; ++i, ++j)
			b58[i] = b58digits_ordered[buf[j]];
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

}
}
#endif