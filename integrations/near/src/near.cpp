#include <marlin/core/Keystore.hpp>
#include <marlin/near/OnRampNear.hpp>
#include <marlin/pubsub/attestation/SigAttester.hpp>

#include <structopt/app.hpp>
#include <unistd.h>


using namespace marlin::core;
using namespace marlin::asyncio;

struct CliOptions {
	std::optional<std::string> discovery_addr;
	std::optional<std::string> pubsub_addr;
	std::optional<std::string> beacon_addr;
	std::optional<std::string> listen_addr;
	std::optional<std::string> keystore_path;
	std::optional<std::string> keystore_pass_path;
	enum class Contracts { mainnet, kovan };
	std::optional<Contracts> contracts;
	std::optional<std::string> chain_identity;
};
STRUCTOPT(CliOptions, discovery_addr, pubsub_addr, beacon_addr, listen_addr, keystore_path, keystore_pass_path, contracts, chain_identity);


int main(int argc, char** argv) {
	try {
		auto options = structopt::app("near_gateway").parse<CliOptions>(argc, argv);
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
			options.discovery_addr.value_or("0.0.0.0:21202")
		);
		auto pubsub_addr = SocketAddress::from_string(
			options.pubsub_addr.value_or("0.0.0.0:21200")
		);
		auto listen_addr = SocketAddress::from_string(
			options.listen_addr.value_or("0.0.0.0:21400")
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
			staking_url = "/subgraphs/name/princesinha19/marlin-staking";
			break;
		};

		SPDLOG_INFO(
			"Starting gateway with discovery: {}, pubsub: {}, listen: {}, beacon: {}, addr: 0x{:spn}",
			discovery_addr.to_string(),
			pubsub_addr.to_string(),
			listen_addr.to_string(),
			beacon_addr.to_string(),
			spdlog::to_hex(key.data(), key.data()+key.size())
		);

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
			"0xa486e4b27cce131bfeacd003018c22a55744bdb94821829f0ff1d4061d8d0533"
		};

		OnRampNear onrampNear(clop, listen_addr, (uint8_t*)key.data());
		return EventLoop::run();
	} catch (structopt::exception& e) {
		SPDLOG_ERROR("{}", e.what());
		SPDLOG_ERROR("{}", e.help());
	}

	return -1;
}

