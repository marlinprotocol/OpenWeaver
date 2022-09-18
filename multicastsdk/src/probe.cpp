#include <sodium.h>
#include <unistd.h>

#include <marlin/multicast/DefaultMulticastClient.hpp>
#include <marlin/pubsub/attestation/SigAttester.hpp>

#include <structopt/app.hpp>


#ifndef MARLIN_PROBE_DEFAULT_PUBSUB_PORT
#define MARLIN_PROBE_DEFAULT_PUBSUB_PORT 15000
#endif

#ifndef MARLIN_PROBE_DEFAULT_DISC_PORT
#define MARLIN_PROBE_DEFAULT_DISC_PORT 15002
#endif

#ifndef MARLIN_PROBE_DEFAULT_NETWORK_ID
#define MARLIN_PROBE_DEFAULT_NETWORK_ID ""
#endif

#ifndef MARLIN_PROBE_DEFAULT_MASK
#define MARLIN_PROBE_DEFAULT_MASK All
#endif

// Pfff, of course macros make total sense!
#define STRH(X) #X
#define STR(X) STRH(X)

#define CONCATH(A, B) A ## B
#define CONCAT(A, B) CONCATH(A, B)


using namespace marlin::multicast;
using namespace marlin::pubsub;
using namespace marlin::core;
using namespace marlin::asyncio;
using namespace marlin::stream;


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


class MulticastDelegate;

using DefaultMulticastClientType = DefaultMulticastClient<
	MulticastDelegate,
	SigAttester,
	LpfBloomWitnesser,
	CONCAT(Mask, MARLIN_PROBE_DEFAULT_MASK)
>;

class MulticastDelegate {
public:
	DefaultMulticastClientType* multicastClient;

	MulticastDelegate(DefaultMulticastClientOptions clop, uint8_t* key) {
		multicastClient = new DefaultMulticastClientType (clop, key);
		multicastClient->delegate = this;
	}

	template<typename T> // TODO: Code smell, remove later
	void did_recv(
		DefaultMulticastClientType &client,
		Buffer &&message,
		T header,
		uint16_t channel,
		uint64_t message_id
	) {
		SPDLOG_DEBUG(
			"Did recv from multicast, message-id: {}",
			message_id
		);
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
};
STRUCTOPT(CliOptions, discovery_addr, pubsub_addr, beacon_addr);


int main(int argc, char** argv) {
	try {
		auto options = structopt::app("probe").parse<CliOptions>(argc, argv);
		auto discovery_addr = SocketAddress::from_string(
			options.discovery_addr.value_or("0.0.0.0:" STR(MARLIN_PROBE_DEFAULT_DISC_PORT))
		);
		auto pubsub_addr = SocketAddress::from_string(
			options.pubsub_addr.value_or("0.0.0.0:" STR(MARLIN_PROBE_DEFAULT_PUBSUB_PORT))
		);
		auto beacon_addr = SocketAddress::from_string(
			options.beacon_addr.value_or("127.0.0.1:8002")
		);

		SPDLOG_INFO(
			"Starting probe with discovery: {}, pubsub: {}, beacon: {}",
			discovery_addr.to_string(),
			pubsub_addr.to_string(),
			beacon_addr.to_string()
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
			"/subgraphs/name/marlinprotocol/staking-arb1",
			STR(MARLIN_PROBE_DEFAULT_NETWORK_ID)
		};

		uint8_t pkey[32] = {};
		MulticastDelegate del(clop, pkey);

		return DefaultMulticastClientType::run_event_loop();
	} catch (structopt::exception& e) {
		SPDLOG_ERROR("{}", e.what());
		SPDLOG_ERROR("{}", e.help());
	}

	return -1;
}
