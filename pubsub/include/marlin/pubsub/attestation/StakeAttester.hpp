#ifndef MARLIN_PUBSUB_ATTESTATION_STAKEATTESTER_HPP
#define MARLIN_PUBSUB_ATTESTATION_STAKEATTESTER_HPP

#include <stdint.h>
#include <marlin/net/Buffer.hpp>
#include <ctime>

#include "marlin/pubsub/ABCInterface.hpp"

#include <secp256k1_recovery.h>
#include <cryptopp/keccak.h>


namespace marlin {
namespace pubsub {

struct StakeAttester {
	ABCInterface& abci;
	uint64_t stake_offset = 0;

	struct Attestation {
		uint64_t message_id;
		uint64_t timestamp;
		uint64_t stake_offset;
		uint16_t channel;
		uint8_t message_hash[32];
		secp256k1_ecdsa_recoverable_signature sig;
	};
	std::vector<Attestation> attestation_cache;
	std::unordered_map<std::string, std::map<uint64_t, Attestation*>> stake_caches;

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
		if(prev_header.attestation_size != 0) {
			out.write(offset, prev_header.attestation_data, prev_header.attestation_size);
			return 1;
		}

		uint64_t timestamp = std::time(nullptr);
		out.write_uint64_be(offset, timestamp);
		out.write_uint64_be(offset+8, stake_offset);
		stake_offset += message_size;

		// TODO: Reset stake_offset and attestation cache at epochs

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

		hasher.TruncatedFinal(hash, 32);

		// Get key
		auto* key = abci.get_key();
		if(key == nullptr) {
			return -1;
		}

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
			return -2;
		}

		// Output
		int recid;
		secp256k1_ecdsa_recoverable_signature_serialize_compact(
			ctx_signer,
			(uint8_t*)out.data()+offset+16,
			&recid,
			&sig
		);

		out.data()[offset+80] = (uint8_t)recid;

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
		auto& attestation = attestation_cache.emplace_back();
		attestation.message_id = message_id;
		attestation.channel = channel;

		// Extract data
		net::Buffer buf((char*)prev_header.attestation_data, prev_header.attestation_size);
		attestation.timestamp = buf.read_uint64_be(0);
		attestation.stake_offset = buf.read_uint64_be(8);
		buf.release();

		uint64_t now = std::time(nullptr);
		// Permit a maximum clock skew of 60 seconds
		if(now > attestation.timestamp && now - attestation.timestamp > 60) {
			// Too old
			return false;
		} else if(now < attestation.timestamp && attestation.timestamp - now > 60) {
			// Too new
			return false;
		}

		CryptoPP::Keccak_256 hasher;
		// Hash message
		hasher.CalculateTruncatedDigest(attestation.message_hash, 32, (uint8_t*)message_data, message_size);

		// Hash for signature
		hasher.Update((uint8_t*)&message_id, 8);  // FIXME: Fix endian
		hasher.Update((uint8_t*)&channel, 2);  // FIXME: Fix endian
		hasher.Update((uint8_t*)prev_header.attestation_data, 16);
		hasher.Update((uint8_t*)&message_size, 8);  // FIXME: Fix endian
		hasher.Update(attestation.message_hash, 32);

		uint8_t hash[32];
		hasher.TruncatedFinal(hash, 32);

		// Parse signature
		secp256k1_ecdsa_recoverable_signature_parse_compact(
			ctx_verifier,
			&attestation.sig,
			(uint8_t*)prev_header.attestation_data + 16,
			(uint8_t)prev_header.attestation_data[80]
		);

		// Verify signature
		secp256k1_pubkey pubkey;
		{
			auto res = secp256k1_ecdsa_recover(
				ctx_verifier,
				&pubkey,
				&attestation.sig,
				hash
			);

			if(res == 0) {
				// Recovery failed
				return false;
			}
		}
		// SPDLOG_INFO("Signature verified");

		auto& stake_cache = stake_caches[std::string(pubkey.data, pubkey.data + 64)];

		auto [iter_begin, res_begin] = stake_cache.try_emplace(
			stake_offset,
			&attestation
		);

		if(!res_begin) {
			// TODO: Handle overlap for iter_begin
			return false;
		}

		auto [iter_end, res_end] = stake_cache.try_emplace(
			stake_offset + message_size - 1,
			&attestation
		);

		if(!res_end) {
			// TODO: Handle overlap for iter_end
			return false;
		}

		bool overlap = false;
		for(iter_begin++; iter_begin != iter_end; iter_begin++) {
			// TODO: Handle overlap for all iters
			overlap = true;
		}

		return !overlap;
	}

	uint64_t parse_size(net::Buffer&, uint64_t = 0) {
		return 81;
	}
};

} // namespace pubsub
} // namespace marlin

#endif // MARLIN_PUBSUB_ATTESTATION_STAKEATTESTER_HPP
