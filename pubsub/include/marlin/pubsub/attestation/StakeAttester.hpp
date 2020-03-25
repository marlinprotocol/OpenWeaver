#ifndef MARLIN_PUBSUB_ATTESTATION_STAKEATTESTER_HPP
#define MARLIN_PUBSUB_ATTESTATION_STAKEATTESTER_HPP

#include <stdint.h>
#include <marlin/net/Buffer.hpp>
#include <cryptopp/cryptlib.h>
#include <cryptopp/eccrypto.h>
#include <cryptopp/osrng.h>
#include <cryptopp/oids.h>


namespace marlin {
namespace pubsub {

struct StakeAttester {
	using CryptoType = CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>;
	using KeyType = CryptoType::PrivateKey;
	KeyType secret_key;

	template<typename HeaderType>
	constexpr uint64_t attestation_size(
		uint64_t,
		uint16_t,
		char const*,
		uint64_t,
		HeaderType
	) {
		return 0;
	}

	template<typename HeaderType>
	int attest(
		uint64_t,
		uint16_t,
		char const*,
		uint64_t,
		HeaderType,
		net::Buffer&,
		uint64_t = 0
	) {
		return 0;
	}

	template<typename HeaderType>
	bool verify(
		uint64_t,
		uint16_t,
		char const*,
		uint64_t,
		HeaderType
	) {
		return true;
	}

	uint64_t parse_size(net::Buffer&, uint64_t = 0) {
		return 0;
	}
};

} // namespace pubsub
} // namespace marlin

#endif // MARLIN_PUBSUB_ATTESTATION_STAKEATTESTER_HPP
