#include <marlin/pubsub/PubSubNode.hpp>
#include <marlin/beacon/DiscoveryServer.hpp>
#include <marlin/beacon/DiscoveryClient.hpp>
#include <unistd.h>


using namespace marlin;
using namespace marlin::core;
using namespace marlin::asyncio;
using namespace marlin::beacon;
using namespace marlin::pubsub;

#define PUBSUB_PROTOCOL_NUMBER 0x10000000

class Goldfish {
public:
	using Self = Goldfish;
	using PubSubNodeType = PubSubNode<
		Self,
		true,
		true,
		true
	>;

	DiscoveryClient<Goldfish> *b;
	PubSubNodeType *ps;

	uint16_t ps_port = 10000;

	std::vector<std::tuple<uint32_t, uint16_t, uint16_t>> get_protocols() {
		return {
			std::make_tuple(PUBSUB_PROTOCOL_NUMBER, 0, ps_port)
		};
	}

	void new_peer(
		core::SocketAddress const &addr,
		uint8_t const* static_pk,
		uint32_t protocol,
		uint16_t
	) {
		if(protocol == PUBSUB_PROTOCOL_NUMBER) {
			ps->subscribe(addr, static_pk);
		}
	}

	std::vector<uint16_t> channels = {0};

	void did_unsubscribe(
		PubSubNodeType &,
		uint16_t channel [[maybe_unused]]
	) {
		SPDLOG_DEBUG("Did unsubscribe: {}", channel);
	}

	void did_subscribe(
		PubSubNodeType &,
		uint16_t channel [[maybe_unused]]
	) {
		SPDLOG_DEBUG("Did subscribe: {}", channel);
	}

	void did_recv(
		PubSubNodeType &,
		Buffer &&,
		typename PubSubNodeType::MessageHeaderType,
		uint16_t channel [[maybe_unused]],
		uint64_t message_id [[maybe_unused]]
	) {
		SPDLOG_INFO(
			"Received message {} on channel {}",
			message_id,
			channel
		);
	}

	bool should_accept(SocketAddress const &) {
		return true;
	}

	void manage_subscriptions(
		size_t,
		typename PubSubNodeType::TransportSet&,
		typename PubSubNodeType::TransportSet&) {
	}
};

int main(int argc, char **argv) {
	std::string beacon_addr("127.0.0.1:9002"),
				heartbeat_addr("127.0.0.1:9003"),
				discovery_addr("127.0.0.1:10002"),
				pubsub_addr("127.0.0.1:10000");

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

	Goldfish g;

	// Beacon
	DiscoveryServer<Goldfish> b(SocketAddress::from_string(beacon_addr), SocketAddress::from_string(heartbeat_addr));
	b.delegate = &g;

	uint8_t static_sk[crypto_box_SECRETKEYBYTES];
	uint8_t static_pk[crypto_box_PUBLICKEYBYTES];
	crypto_box_keypair(static_pk, static_sk);

	// Pubsub
	PubSubNode<
		Goldfish,
		true,
		true,
		true
	> ps(SocketAddress::from_string(pubsub_addr), 1000, 1000, static_sk);
	ps.delegate = &g;
	g.ps = &ps;

	// Discovery client
	DiscoveryClient<Goldfish> dc(SocketAddress::from_string(discovery_addr), static_sk);
	dc.delegate = &g;
	dc.is_discoverable = true;
	g.b = &dc;

	dc.start_discovery(SocketAddress::from_string(beacon_addr));

	return EventLoop::run();
}
