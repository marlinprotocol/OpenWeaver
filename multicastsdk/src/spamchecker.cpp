#include <sodium.h>
#include <unistd.h>

#include <marlin/core/Keystore.hpp>
#include <marlin/multicast/DefaultMulticastClient.hpp>
#include <marlin/matic/Abci.hpp>
#include <marlin/cosmos/Abci.hpp>
#include <marlin/eth/Abci.hpp>
#include <marlin/pubsub/attestation/SigAttester.hpp>

#include <structopt/app.hpp>


#ifndef MARLIN_SC_DEFAULT_PUBSUB_PORT
#define MARLIN_SC_DEFAULT_PUBSUB_PORT 15000
#endif

#ifndef MARLIN_SC_DEFAULT_DISC_PORT
#define MARLIN_SC_DEFAULT_DISC_PORT 15002
#endif

#ifndef MARLIN_SC_DEFAULT_LISTEN_PORT
#define MARLIN_SC_DEFAULT_LISTEN_PORT 15003
#endif

#ifndef MARLIN_SC_DEFAULT_NETWORK_ID
#define MARLIN_SC_DEFAULT_NETWORK_ID ""
#endif

#ifndef MARLIN_SC_DEFAULT_MASK
#define MARLIN_SC_DEFAULT_MASK All
#endif

#ifndef MARLIN_SC_DEFAULT_CHAIN
#define MARLIN_SC_DEFAULT_CHAIN nil
#endif

// Pfff, of course macros make total sense!
#define STRH(X) #X
#define STR(X) STRH(X)

#define CONCATH(A, B) A ## B
#define CONCAT(A, B) CONCATH(A, B)


using namespace marlin::multicast;
using namespace marlin::pubsub;
using namespace marlin::core;


struct MaskCosmosv1 {
	static uint64_t mask(
		WeakBuffer buf
	) {
		// msg type
		auto type = buf.read_uint8_unsafe(1);

		// block check
		if(type == 0x90) {
			return 0x0;
		}

		return 0xff;
	}
};


struct MulticastDelegate;

using DefaultMulticastClientType = DefaultMulticastClient<
	MulticastDelegate,
	SigAttester,
	LpfBloomWitnesser,
	CONCAT(Mask, MARLIN_SC_DEFAULT_MASK)
>;

class MulticastDelegate {
public:
	DefaultMulticastClientType multicastClient;
	using AbciType = marlin::MARLIN_SC_DEFAULT_CHAIN::Abci<MulticastDelegate, uint64_t>;
	AbciType abci;

	MulticastDelegate(DefaultMulticastClientOptions clop, std::string abci_addr, uint8_t* key) :
		abci(abci_addr),
		multicastClient(clop, key) {
		multicastClient.delegate = this;

		abci.delegate = this;
	}

	bool is_connected = false;

	void did_connect(AbciType& abci) {
		is_connected = true;
	}

	void did_disconnect(AbciType&) {
		is_connected = false;
	}

	void did_close(AbciType&) {
		// Should not happen
		std::terminate();
	}

	void did_analyze_block(
		AbciType&,
		Buffer&&,
		std::string,
		std::string,
		WeakBuffer,
		uint64_t message_id
	) {
		SPDLOG_INFO(
			"Spam checked message: {}", message_id
		);
	}

	void did_recv(
		DefaultMulticastClientType &,
		Buffer &&message,
		auto&&,
		uint16_t,
		uint64_t message_id
	) {
		SPDLOG_DEBUG(
			"Did recv from multicast, message-id: {}",
			message_id
		);

		if(!is_connected) {
			SPDLOG_ERROR("Abci not active, dropping block");
			return;
		}

		if((message_id & CONCAT(Mask, MARLIN_SC_DEFAULT_MASK)::mask(message)) == 0) {
			abci.analyze_block(std::move(message), message_id);
		}
	}

	void did_subscribe(
		DefaultMulticastClientType &client,
		uint16_t channel
	) {}

	void did_unsubscribe(
		DefaultMulticastClientType &client,
		uint16_t channel
	) {}
};

struct CliOptions {
	std::optional<std::string> discovery_addr;
	std::optional<std::string> pubsub_addr;
	std::optional<std::string> beacon_addr;
	std::optional<std::string> listen_addr;
	std::optional<std::string> keystore_path;
	std::optional<std::string> keystore_pass_path;
	enum class Contracts { mainnet, kovan };
	std::optional<Contracts> contracts;
};
STRUCTOPT(CliOptions, discovery_addr, pubsub_addr, beacon_addr, listen_addr, keystore_path, keystore_pass_path, contracts);


int main(int argc, char** argv) {
	try {
		auto options = structopt::app("bridge").parse<CliOptions>(argc, argv);
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
			options.discovery_addr.value_or("0.0.0.0:" STR(MARLIN_SC_DEFAULT_DISC_PORT))
		);
		auto pubsub_addr = SocketAddress::from_string(
			options.pubsub_addr.value_or("0.0.0.0:" STR(MARLIN_SC_DEFAULT_PUBSUB_PORT))
		);
		auto beacon_addr = SocketAddress::from_string(
			options.beacon_addr.value_or("127.0.0.1:8002")
		);

		std::string staking_url;
		switch(options.contracts.value_or(CliOptions::Contracts::mainnet)) {
		case CliOptions::Contracts::mainnet:
			staking_url = "/subgraphs/name/marlinprotocol/staking";
			break;
		case CliOptions::Contracts::kovan:
			staking_url = "/subgraphs/name/marlinprotocol/staking-kovan";
			break;
		};

		SPDLOG_INFO(
			"Starting bridge with discovery: {}, pubsub: {}, listen: {}, beacon: {}",
			discovery_addr.to_string(),
			pubsub_addr.to_string(),
			options.listen_addr.value_or("nil"),
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
			STR(MARLIN_SC_DEFAULT_NETWORK_ID)
		};

		MulticastDelegate del(clop, options.listen_addr.value_or("127.0.0.1:8002"), (uint8_t*)key.data());

		return DefaultMulticastClientType::run_event_loop();
	} catch (structopt::exception& e) {
		SPDLOG_ERROR("{}", e.what());
		SPDLOG_ERROR("{}", e.help());
	}

	return -1;
}

