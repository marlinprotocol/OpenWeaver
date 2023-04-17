#ifndef MARLIN_PUBSUB_ATTESTATION_SIGATTESTER_HPP
#define MARLIN_PUBSUB_ATTESTATION_SIGATTESTER_HPP

#include <stdint.h>
#include <marlin/core/WeakBuffer.hpp>
#include <ctime>
#include <optional>

#include <secp256k1_recovery.h>
#include <cryptopp/keccak.h>
#include <cryptopp/osrng.h>


namespace marlin {
namespace pubsub {

struct SigAttester {
	secp256k1_context* ctx_signer = nullptr;
	secp256k1_context* ctx_verifier = nullptr;
	uint8_t key[32];

	SigAttester(uint8_t* key) {
		ctx_signer = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
		ctx_verifier = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);

		std::memcpy(this->key, key, 32);
	}

	~SigAttester() {
		secp256k1_context_destroy(ctx_signer);
		secp256k1_context_destroy(ctx_verifier);
	}

	template<typename HeaderType>
	constexpr uint64_t attestation_size(
		uint64_t,
		uint16_t,
		uint8_t const*,
		uint64_t,
		HeaderType
	) {
		return 67;
	}

	template<typename HeaderType>
	int attest(
		uint64_t,
		uint16_t,
		uint8_t const* message_data,
		uint64_t message_size,
		HeaderType prev_header,
		core::Buffer& out,
		uint64_t offset = 0
	) {
		if(prev_header.attestation_size != 0) {
			// FIXME: should probably add _unsafe to function
			out.write_unsafe(offset, prev_header.attestation_data, prev_header.attestation_size);
			return 1;
		}

		out.write_uint16_le_unsafe(offset, 67);

		uint8_t hash[32];
		CryptoPP::Keccak_256 hasher;
		// Hash message
		hasher.CalculateTruncatedDigest(hash, 32, message_data, message_size);

		// Get key
		// if(key == nullptr) {
		// 	return -2;
		// }

		// Sign
		secp256k1_ecdsa_recoverable_signature sig;
		auto res = secp256k1_ecdsa_sign_recoverable(
			ctx_signer,
			&sig,
			hash,
			key,
			nullptr,
			nullptr
		);

		if(res == 0) {
			// Sign failed
			return -3;
		}

		// Output
		int recid;
		secp256k1_ecdsa_recoverable_signature_serialize_compact(
			ctx_signer,
			out.data()+offset+2,
			&recid,
			&sig
		);

		out.data()[offset+66] = (uint8_t)recid;

		return 0;
	}

	template<typename HeaderType>
	std::tuple<bool, std::array<uint8_t, 20>> verify(
		uint64_t message_id,
		uint16_t channel,
		uint8_t const* message_data,
		uint64_t message_size,
		HeaderType prev_header
	) {
		uint8_t hash[32];
		CryptoPP::Keccak_256 hasher;
		// Hash message
		hasher.CalculateTruncatedDigest(hash, 32, message_data, message_size);

		secp256k1_ecdsa_recoverable_signature sig;
		
		// Parse signature
		secp256k1_ecdsa_recoverable_signature_parse_compact(
			ctx_verifier,
			&sig,
			prev_header.attestation_data + 2,
			prev_header.attestation_data[66]
		);
		
		// Verify signature
		std::array<uint8_t, 20> address;
		secp256k1_pubkey pubkey;
		{
			auto res = secp256k1_ecdsa_recover(
				ctx_verifier,
				&pubkey,
				&sig,
				hash
			);

			if(res == 0) {
				// Recovery failed
				return make_tuple(false, address);
			}
		}

		// Get address
		hasher.CalculateTruncatedDigest(hash, 32, pubkey.data, 64);
		// address is in hash[12..31]
		for(int i=0;i<20;i++){
			address[i] = hash[i+12];
		}

		//SPDLOG_INFO("pubkey: {}", spdlog::to_hex(pubkey.data, pubkey.data+64));
		
		return make_tuple(true, address);
	}

	std::optional<uint64_t> parse_size(core::Buffer& buf, uint64_t offset = 0) {
		if(buf.read_uint16_le(offset) == 67) {
			return 67;
		}
		return std::nullopt;
	}
};

} // namespace pubsub
} // namespace marlin

#endif // MARLIN_PUBSUB_ATTESTATION_SIGATTESTER_HPP
