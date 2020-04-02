#ifndef MARLIN_PUBSUB_ATTESTATION_STAKEATTESTER_HPP
#define MARLIN_PUBSUB_ATTESTATION_STAKEATTESTER_HPP

#include <stdint.h>
#include <marlin/net/Buffer.hpp>
#include <ctime>
#include <optional>

#include "marlin/pubsub/ABCInterface.hpp"

#include <secp256k1_recovery.h>
#include <cryptopp/keccak.h>


namespace marlin {
namespace pubsub {

struct StakeAttester {
	ABCInterface& abci;

	secp256k1_context* ctx_signer = nullptr;
	secp256k1_context* ctx_verifier = nullptr;

//---------------- Self stake management start ----------------//

	// Circular allocator
	// NOTE: Never let free_stake_offset == used_stake_offset, always has to be atleast 1 ahead
	uint64_t free_stake_offset = 0;
	uint64_t used_stake_offset = -1;
	// Timestamp -> Size
	std::map<uint64_t, uint64_t> used_stake;

	void stake_reclaim(uint64_t timestamp) {
		auto end = used_stake.upper_bound(timestamp);
		for(auto iter = used_stake.begin(); iter != end; iter = used_stake.erase(iter)) {
			used_stake_offset += iter->second;
		}
	}

	std::optional<uint64_t> stake_alloc(uint64_t size) {
		// Get key
		auto* key = abci.get_key();
		if(key == nullptr) {
			return std::nullopt;
		}

		// Get pubkey
		secp256k1_pubkey pubkey;
		auto res = secp256k1_ec_pubkey_create(
			ctx_signer,
			&pubkey,
			key
		);
		if(res == 0) {
			// Pubkey failed
			return std::nullopt;
		}

		// Get address
		uint8_t hash[32];
		CryptoPP::Keccak_256 hasher;
		hasher.CalculateTruncatedDigest(hash, 32, pubkey.data, 64);
		// address is in hash[12..31]

		// Check if stake_offset is within stake
		auto stake = abci.get_stake(std::string((char*)hash+12, 20));
		if(used_stake_offset > stake) {
			used_stake_offset = stake;
		}
		if(free_stake_offset >= used_stake_offset) {
			free_stake_offset = (used_stake_offset + 1) % stake;
		}

		if(used_stake_offset > (free_stake_offset + size) % stake) {  // Overflow behaviour desirable
			auto ret = std::make_optional(free_stake_offset);
			free_stake_offset = (free_stake_offset + size) % stake;

			return ret;
		}

		return std::nullopt;
	}

//---------------- Self stake management end ----------------//


//---------------- Other stake management start ----------------//

	struct Attestation {
		uint64_t message_id;
		uint64_t timestamp;
		uint64_t stake_offset;
		uint64_t message_size;
		uint16_t channel;
		uint8_t message_hash[32];
		secp256k1_ecdsa_recoverable_signature sig;
	};
	std::vector<Attestation> attestation_cache;
	std::unordered_map<std::string, std::map<uint64_t, Attestation*>> stake_caches;

//---------------- Other stake management end ----------------//

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
		uint8_t const*,
		uint64_t,
		HeaderType
	) {
		return 81;
	}

	template<typename HeaderType>
	int attest(
		uint64_t message_id,
		uint16_t channel,
		uint8_t const* message_data,
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

		// TODO: Should I be calling reclaim everytime? Better ways? Periodic timer?
		stake_reclaim(timestamp - 60);
		auto stake_offset_opt = stake_alloc(message_size);
		if(!stake_offset_opt.has_value()) {
			return -1;
		}
		auto stake_offset = stake_offset_opt.value();

		out.write_uint64_be(offset, timestamp);
		out.write_uint64_be(offset+8, stake_offset);

		uint8_t hash[32];
		CryptoPP::Keccak_256 hasher;
		// Hash message
		hasher.CalculateTruncatedDigest(hash, 32, message_data, message_size);

		// Hash for signature
		hasher.Update((uint8_t*)&message_id, 8);  // FIXME: Fix endian
		hasher.Update((uint8_t*)&channel, 2);  // FIXME: Fix endian
		hasher.Update(out.data()+offset, 16);
		hasher.Update((uint8_t*)&message_size, 8);  // FIXME: Fix endian
		hasher.Update(hash, 32);

		hasher.TruncatedFinal(hash, 32);

		// Get key
		auto* key = abci.get_key();
		if(key == nullptr) {
			return -2;
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
			return -3;
		}

		// Output
		int recid;
		secp256k1_ecdsa_recoverable_signature_serialize_compact(
			ctx_signer,
			out.data()+offset+16,
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
		uint8_t const* message_data,
		uint64_t message_size,
		HeaderType prev_header
	) {
		auto& attestation = attestation_cache.emplace_back();
		attestation.message_id = message_id;
		attestation.channel = channel;
		attestation.message_size = message_size;

		// Extract data
		net::Buffer buf((uint8_t*)prev_header.attestation_data, prev_header.attestation_size);
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
		hasher.CalculateTruncatedDigest(attestation.message_hash, 32, message_data, message_size);

		// Hash for signature
		hasher.Update((uint8_t*)&message_id, 8);  // FIXME: Fix endian
		hasher.Update((uint8_t*)&channel, 2);  // FIXME: Fix endian
		hasher.Update(prev_header.attestation_data, 16);
		hasher.Update((uint8_t*)&message_size, 8);  // FIXME: Fix endian
		hasher.Update(attestation.message_hash, 32);

		uint8_t hash[32];
		hasher.TruncatedFinal(hash, 32);

		// Parse signature
		secp256k1_ecdsa_recoverable_signature_parse_compact(
			ctx_verifier,
			&attestation.sig,
			prev_header.attestation_data + 16,
			prev_header.attestation_data[80]
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

		// Get address
		hasher.CalculateTruncatedDigest(hash, 32, pubkey.data, 64);
		// address is in hash[12..31]

		// Check if stake_offset is within stake
		auto stake = abci.get_stake(std::string((char*)hash+12, 20));
		if(attestation.stake_offset > stake || attestation.stake_offset + attestation.message_size > stake) {
			return false;
		}

		// Check for overlaps
		auto& stake_cache = stake_caches[std::string(pubkey.data, pubkey.data + 64)];

		auto [iter_begin, res_begin] = stake_cache.try_emplace(
			attestation.stake_offset,
			&attestation
		);

		if(!res_begin) {
			abci.send_duplicate_stake_msg(
				attestation.message_id,
				attestation.channel,
				attestation.timestamp,
				attestation.stake_offset,
				attestation.message_size,
				attestation.message_hash,
				attestation.sig.data,
				iter_begin->second->message_id,
				iter_begin->second->channel,
				iter_begin->second->timestamp,
				iter_begin->second->stake_offset,
				iter_begin->second->message_size,
				iter_begin->second->message_hash,
				iter_begin->second->sig.data
			);
			return false;
		}

		auto [iter_end, res_end] = stake_cache.try_emplace(
			attestation.stake_offset + message_size - 1,
			&attestation
		);

		if(!res_end) {
			abci.send_duplicate_stake_msg(
				attestation.message_id,
				attestation.channel,
				attestation.timestamp,
				attestation.stake_offset,
				attestation.message_size,
				attestation.message_hash,
				attestation.sig.data,
				iter_end->second->message_id,
				iter_end->second->channel,
				iter_end->second->timestamp,
				iter_end->second->stake_offset,
				iter_end->second->message_size,
				iter_end->second->message_hash,
				iter_end->second->sig.data
			);
			return false;
		}

		bool overlap = false;
		for(iter_begin++; iter_begin != iter_end; iter_begin++) {
			abci.send_duplicate_stake_msg(
				attestation.message_id,
				attestation.channel,
				attestation.timestamp,
				attestation.stake_offset,
				attestation.message_size,
				attestation.message_hash,
				attestation.sig.data,
				iter_begin->second->message_id,
				iter_begin->second->channel,
				iter_begin->second->timestamp,
				iter_begin->second->stake_offset,
				iter_begin->second->message_size,
				iter_begin->second->message_hash,
				iter_begin->second->sig.data
			);
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
