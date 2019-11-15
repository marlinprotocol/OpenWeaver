#include <marlin/pubsub/PubSubServer.hpp>
#include <marlin/beacon/DiscoveryServer.hpp>
#include <marlin/beacon/DiscoveryClient.hpp>
#include <uv.h>


using namespace marlin;
using namespace marlin::net;
using namespace marlin::beacon;
using namespace marlin::pubsub;

#define PUBSUB_PROTOCOL_NUMBER 0x10000000

class Goldfish {
public:
	marlin::beacon::DiscoveryClient<Goldfish> *b;
	marlin::pubsub::PubSubServer<Goldfish> *ps;

	uint16_t ps_port = 10000;

	std::vector<std::tuple<uint32_t, uint16_t, uint16_t>> get_protocols() {
		// uint16_t ps_port = ps->
		return {
			std::make_tuple(PUBSUB_PROTOCOL_NUMBER, 0, ps_port)
		};
	}

	void new_peer(
		net::SocketAddress const &addr,
		uint32_t protocol,
		uint16_t
	) {
		if(protocol == PUBSUB_PROTOCOL_NUMBER) {
			ps->subscribe(addr);
		}
	}

	std::vector<std::string> channels = {"goldfish"};

	void did_unsubscribe(
		marlin::pubsub::PubSubServer<Goldfish> &,
		std::string channel __attribute__((unused))
	) {
		SPDLOG_DEBUG("Did unsubscribe: {}", channel);
	}

	void did_subscribe(
		marlin::pubsub::PubSubServer<Goldfish> &,
		std::string channel __attribute__((unused))
	) {
		SPDLOG_DEBUG("Did subscribe: {}", channel);
	}

	void did_recv_message(
		marlin::pubsub::PubSubServer<Goldfish> &,
		Buffer &&,
		std::string &channel __attribute__((unused)),
		uint64_t message_id __attribute__((unused))
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

	void manage_subscribers(
		std::string,
		typename marlin::pubsub::PubSubServer<Goldfish>::TransportSet&,
		typename marlin::pubsub::PubSubServer<Goldfish>::TransportSet&) {
	}
};

int main(int argc, char **argv) {
	std::string beacon_addr("127.0.0.1:9002"),
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
	DiscoveryServer<Goldfish> b(SocketAddress::from_string(beacon_addr));
	b.delegate = &g;

	// Pubsub
	PubSubServer<Goldfish> ps(SocketAddress::from_string(pubsub_addr));
	ps.delegate = &g;
	ps.should_relay = true;
	g.ps = &ps;

	// Discovery client
	DiscoveryClient<Goldfish> dc(SocketAddress::from_string(discovery_addr));
	dc.delegate = &g;
	dc.is_discoverable = true;
	g.b = &dc;

	dc.start_discovery(SocketAddress::from_string(beacon_addr));

	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
