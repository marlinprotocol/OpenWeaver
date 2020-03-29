#ifndef MARLIN_PUBSUB_ATTESTATION_STAKEATTESTER_HPP
#define MARLIN_PUBSUB_ATTESTATION_STAKEATTESTER_HPP

#include <stdint.h>
#include <marlin/net/Buffer.hpp>
#include <ctime>

#include "marlin/pubsub/ABCInterface.hpp"

#include <secp256k1_preallocated.h>

namespace marlin {
namespace pubsub {

struct StakeAttester {
	ABCInterface& abci;
	uint64_t stake_offset = 0;

	secp256k1_context* ctx_signer = nullptr;
	secp256k1_context* ctx_verifier = nullptr;

	StakeAttester(ABCInterface& abci) : abci(abci) {
		ctx_signer = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
		ctx_verifier = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);
	}

	~StakeAttester() {
		secp256k1_context_destroy(ctx_signer);
		secp256k1_context_destroy(ctx_verifier);
	}

	template<typename HeaderType>
	constexpr uint64_t attestation_size(
		uint64_t,
		uint16_t,
		char const*,
		uint64_t,
		HeaderType
	) {
		return 81;
	}

	template<typename HeaderType>
	int attest(
		uint64_t message_id,
		uint16_t channel,
		char const* message_data,
		uint64_t message_size,
		HeaderType prev_header,
		net::Buffer& out,
		uint64_t offset = 0
	) {
		if(prev_header.attestation_data != nullptr) {
			out.write(offset, prev_header.attestation_data, prev_header.attestation_size);
			return 1;
		}

		uint64_t timestamp = std::time(nullptr);
		out.write_uint64_be(offset, timestamp);
		out.write_uint64_be(offset+8, stake_offset);
		stake_offset += message_size;

		// TODO: Rotate stake_offset at epochs

		uint8_t hash[32];
		CryptoPP::Keccak_256 hasher;
		// Hash message
		hasher.CalculateTruncatedDigest(hash, 32, (uint8_t*)message_data, message_size);

		// Hash for signature
		hasher.Update((uint8_t*)&message_id, 8);  // FIXME: Fix endian
		hasher.Update((uint8_t*)&channel, 2);  // FIXME: Fix endian
		hasher.Update((uint8_t*)out.data()+offset, 16);
		hasher.Update((uint8_t*)&message_size, 8);  // FIXME: Fix endian
		hasher.Update(hash, 32);

		// Get key
		auto* key = abci.get_key();
		if(key == nullptr) {
			return -1;
		}

		// Sign
		secp256k1_ecdsa_signature sig;
		secp256k1_ecdsa_sign(
			ctx_signer,
			&sig,
			hash,
			key,
			nullptr,
			nullptr
		);

		// Output
		secp256k1_ecdsa_signature_serialize_compact(
			ctx_signer,
			(uint8_t*)out.data()+offset+16,
			&sig
		);

		// TODO: Output recovery

		return 0;
	}

	template<typename HeaderType>
	bool verify(
		uint64_t message_id,
		uint16_t channel,
		char const* message_data,
		uint64_t message_size,
		HeaderType prev_header
	) {
		if(prev_header.attestation_size < 81) {
			return false;
		}

		// Extract data
		net::Buffer attestation((char*)prev_header.attestation_data, prev_header.attestation_size);
		auto timestamp = attestation.read_uint64_be(0);
		auto stake_offset = attestation.read_uint64_be(8);
		attestation.release();

		uint64_t now = std::time(nullptr);
		// Permit a maximum clock skew of 60 seconds
		if(now > timestamp && now - timestamp > 60) {
			// Too old
			return false;
		} else if(now < timestamp && timestamp - now > 60) {
			// Too new
			return false;
		}

		uint8_t hash[32];
		CryptoPP::Keccak_256 hasher;
		// Hash message
		hasher.CalculateTruncatedDigest(hash, 32, (uint8_t*)message_data, message_size);

		// Hash for signature
		hasher.Update((uint8_t*)&message_id, 8);  // FIXME: Fix endian
		hasher.Update((uint8_t*)&channel, 2);  // FIXME: Fix endian
		hasher.Update((uint8_t*)prev_header.attestation_data, 16);
		hasher.Update((uint8_t*)&message_size, 8);  // FIXME: Fix endian
		hasher.Update(hash, 32);

		// Parse signature
		secp256k1_ecdsa_signature sig;
		secp256k1_ecdsa_signature_parse_compact(
			ctx_verifier,
			&sig,
			(uint8_t*)prev_header.attestation_data + 10
		);

		// Verify signature
		secp256k1_pubkey pubkey;
		auto res = secp256k1_ecdsa_verify(
			ctx_verifier,
			&sig,
			hash,
			&pubkey
		);

		if(res == 0) {
			// Verification failed
			return false;
		}

		// TODO: Cache stake and check for duplicates
		(void)stake_offset;

		return true;
	}

	uint64_t parse_size(net::Buffer&, uint64_t = 0) {
		return 81;
	}
};

} // namespace pubsub
} // namespace marlin

#endif // MARLIN_PUBSUB_ATTESTATION_STAKEATTESTER_HPP
