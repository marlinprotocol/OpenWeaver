#include <marlin/near/OnRampNear.hpp>
#include <unistd.h>


using namespace marlin::core;
using namespace marlin::asyncio;

int main(int argc, char **argv) {
	std::string beacon_addr = "127.0.0.1:8002";
	std::string discovery_addr = "0.0.0.0:21202";
	std::string pubsub_addr = "0.0.0.0:21200";

	char c;
	while ((c = getopt (argc, argv, "b::d::p::")) != -1) {
		switch (c) {
			case 'b':
				beacon_addr = std::string(optarg);
				break;
			case 'd':
				discovery_addr = std::string(optarg);
				break;
			case 'p':
				pubsub_addr = std::string(optarg);
				break;
			default:
			return 1;
		}
	}

	SPDLOG_INFO(
		"Beacon: {}, Discovery: {}, PubSub: {}",
		beacon_addr,
		discovery_addr,
		pubsub_addr
	);

	uint8_t static_sk[crypto_box_SECRETKEYBYTES];
	uint8_t static_pk[crypto_box_PUBLICKEYBYTES];
	crypto_box_keypair(static_pk, static_sk);
	DefaultMulticastClientOptions clop {
		static_sk,
		static_pk,
		std::vector<uint16_t>({0, 1}),
		beacon_addr,
		discovery_addr,
		pubsub_addr
	};

	OnRampNear onrampNear(clop);
	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
