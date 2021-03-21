#include <sodium.h>
#include <unistd.h>

#include <marlin/pubsub/witness/BloomWitnesser.hpp>
#include <marlin/multicast/DefaultMulticastClient.hpp>


using namespace marlin::multicast;
using namespace marlin::core;
using namespace marlin::asyncio;
using namespace marlin::stream;
using namespace marlin::lpf;
using namespace marlin::pubsub;


class MulticastDelegate {
public:
	DefaultMulticastClient<MulticastDelegate, EmptyAttester, BloomWitnesser>* multicastClient;

	MulticastDelegate(DefaultMulticastClientOptions clop) {
		multicastClient = new DefaultMulticastClient<MulticastDelegate, EmptyAttester, BloomWitnesser> (clop);
		multicastClient->delegate = this;
	}

	template<typename T> // TODO: Code smell, remove later
	void did_recv(
		DefaultMulticastClient<MulticastDelegate, EmptyAttester, BloomWitnesser> &client,
		Buffer &&message,
		T header,
		uint16_t channel,
		uint64_t message_id
	) {
		SPDLOG_INFO(
			"Received message {} on channel {}",
			message_id,
			channel
		);
	}

	void did_subscribe(
		DefaultMulticastClient<MulticastDelegate, EmptyAttester, BloomWitnesser> &client,
		uint16_t channel
	) {}

	void did_unsubscribe(
		DefaultMulticastClient<MulticastDelegate, EmptyAttester, BloomWitnesser> &client,
		uint16_t channel
	) {}
};

int main(int argc, char **argv) {

	std::string beacon_addr = "0.0.0.0:90002";
	std::string discovery_addr = "0.0.0.0:15002";
	std::string pubsub_addr = "0.0.0.0:15000";
	std::string lpftcp_bridge_addr = "0.0.0.0:15003";

	char c;
	while ((c = getopt (argc, argv, "b::d::p::l::")) != -1) {
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

	MulticastDelegate del(clop);

	return DefaultMulticastClient<MulticastDelegate, EmptyAttester, BloomWitnesser>::run_event_loop();
}
