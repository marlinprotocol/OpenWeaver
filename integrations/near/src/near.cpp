#include <marlin/near/OnRampNear.hpp>
#include <unistd.h>


using namespace marlin::core;
using namespace marlin::asyncio;

std::pair <std::string, std::string> split_by_delim(std::string data, char delim) {
	std::string fs = "", ss = "";
	bool flag = false;
	for(int i = 0; i < int(data.size()); i++) {
		if(data[i] == delim) {
			flag = true;
			continue;
		}
		if(!flag) {
			fs += data[i];
		} else {
			ss += data[i];
		}
	}
	return {fs, ss};
}

int main(int argc, char **argv) {
	std::string beacon_addr = "127.0.0.1:8002";
	std::string discovery_addr = "0.0.0.0:21202";
	std::string pubsub_addr = "0.0.0.0:21200";
	std::string near_addr = "1";

	// char c;
	// while ((c = getopt (argc, argv, "b::d::p::")) != -1) {
	// 	switch (c) {
	// 		case 'b':
	// 			beacon_addr = std::string(optarg);
	// 			break;
	// 		case 'd':
	// 			discovery_addr = std::string(optarg);
	// 			break;
	// 		case 'p':
	// 			pubsub_addr = std::string(optarg);
	// 			break;
	// 		default:
	// 		return 1;
	// 	}
	// }
	std::string bn_address = "";
	for(int i = 1; i < argc - 1; i++) {
		if(std::string(argv[i]) == "--boot-nodes") {
			bn_address = std::string(argv[i + 1]);
		}
	}

	std::string near_key, run_port;
	std::tie(near_key, run_port) = split_by_delim(bn_address, '@');

	SPDLOG_INFO(
		"Beacon: {}, Discovery: {}, PubSub: {}, NearAddr: {}",
		beacon_addr,
		discovery_addr,
		pubsub_addr,
		near_addr
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

	OnRampNear onrampNear(clop, near_key, run_port);
	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}