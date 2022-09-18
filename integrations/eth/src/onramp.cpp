#include "OnRamp.hpp"
#include <unistd.h>

#include <marlin/core/Keystore.hpp>
#include <structopt/app.hpp>


using namespace marlin::core;
using namespace marlin::asyncio;
using namespace marlin::beacon;
using namespace marlin::pubsub;
using namespace marlin::rlpx;

struct CliOptions {
	std::optional<std::string> discovery_addr;
	std::optional<std::string> pubsub_addr;
	std::optional<std::string> beacon_addr;
	std::optional<std::string> keystore_path;
	std::optional<std::string> keystore_pass_path;
	enum class Contracts { mainnet, kovan };
	std::optional<Contracts> contracts;
};
STRUCTOPT(CliOptions, discovery_addr, pubsub_addr, beacon_addr, keystore_path, keystore_pass_path, contracts);


int main(int argc, char** argv) {
	try {
		auto options = structopt::app("gateway").parse<CliOptions>(argc, argv);
		std::string key;
		if(options.beacon_addr.has_value()) {
			if(options.keystore_path.has_value() && options.keystore_pass_path.has_value()) {
				key = get_key(options.keystore_path.value(), options.keystore_pass_path.value());
				if (key.empty()) {
					SPDLOG_ERROR("keystore file error");
					return 1;
				}
			} else {
				SPDLOG_ERROR("require keystore and password file");
				return 1;
			}
		}
		auto discovery_addr = SocketAddress::from_string(
			options.discovery_addr.value_or("0.0.0.0:15002")
		);
		auto pubsub_addr = SocketAddress::from_string(
			options.pubsub_addr.value_or("0.0.0.0:15000")
		);
		auto beacon_addr = SocketAddress::from_string(
			options.beacon_addr.value_or("127.0.0.1:8002")
		);

		std::string staking_url;
		switch(options.contracts.value_or(CliOptions::Contracts::mainnet)) {
		case CliOptions::Contracts::mainnet:
			staking_url = "/subgraphs/name/marlinprotocol/staking-arb1";
			break;
		case CliOptions::Contracts::kovan:
			staking_url = "/subgraphs/name/marlinprotocol/staking-kovan";
			break;
		};

		SPDLOG_INFO(
			"Starting gateway with discovery: {}, pubsub: {}, beacon: {}",
			discovery_addr.to_string(),
			pubsub_addr.to_string(),
			beacon_addr.to_string()
		);

		{
			if(key.size() == 0) {
				SPDLOG_INFO("Bridge: No identity key provided");
			} else if(key.size() == 32) {
				auto* ctx_signer = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
				auto* ctx_verifier = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);
				if(secp256k1_ec_seckey_verify(ctx_verifier, (uint8_t*)key.c_str()) != 1) {
					SPDLOG_ERROR("Bridge: failed to verify key", key.size());
					secp256k1_context_destroy(ctx_verifier);
					secp256k1_context_destroy(ctx_signer);
					return -1;
				}

				secp256k1_pubkey pubkey;
				auto res = secp256k1_ec_pubkey_create(
					ctx_signer,
					&pubkey,
					(uint8_t*)key.c_str()
				);
				(void)res;

				uint8_t pubkeyser[65];
				size_t len = 65;
				secp256k1_ec_pubkey_serialize(
					ctx_signer,
					pubkeyser,
					&len,
					&pubkey,
					SECP256K1_EC_UNCOMPRESSED
				);

				// Get address
				uint8_t hash[32];
				CryptoPP::Keccak_256 hasher;
				hasher.CalculateTruncatedDigest(hash, 32, pubkeyser+1, 64);
				// address is in hash[12..31]

				SPDLOG_INFO("Bridge: Identity is 0x{:spn}", spdlog::to_hex(hash+12, hash+32));

				secp256k1_context_destroy(ctx_verifier);
				secp256k1_context_destroy(ctx_signer);
			} else {
				SPDLOG_ERROR("Bridge: failed to load key: {}", key.size());
			}
		}

		uint8_t static_sk[crypto_box_SECRETKEYBYTES];
		uint8_t static_pk[crypto_box_PUBLICKEYBYTES];
		crypto_box_keypair(static_pk, static_sk);

		DefaultMulticastClientOptions clop {
			static_sk,
			static_pk,
			std::vector<uint16_t>({0, 1}),
			beacon_addr.to_string(),
			discovery_addr.to_string(),
			pubsub_addr.to_string(),
			staking_url,
		};

		OnRamp onramp(clop, (uint8_t*)key.data());

		return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
	} catch (structopt::exception& e) {
		SPDLOG_ERROR("{}", e.what());
		SPDLOG_ERROR("{}", e.help());
	}

	return -1;
}

