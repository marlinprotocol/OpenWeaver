#ifndef MARLIN_RELAY_RELAY_HPP
#define MARLIN_RELAY_RELAY_HPP

#include <marlin/pubsub/PubSubNode.hpp>
#include <marlin/beacon/DiscoveryClient.hpp>

using namespace marlin;
using namespace marlin::net;
using namespace marlin::beacon;
using namespace marlin::pubsub;

#define PUBSUB_PROTOCOL_NUMBER 0x10000000

class Relay {
public:
	marlin::beacon::DiscoveryClient<Relay> *b;
	marlin::pubsub::PubSubNode<Relay> *ps;

	std::vector<std::tuple<uint32_t, uint16_t, uint16_t>> get_protocols() {
		return {
			std::make_tuple(PUBSUB_PROTOCOL_NUMBER, 0, 8000)
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

	std::vector<std::string> channels = {"eth"};

	void did_unsubscribe(
		marlin::pubsub::PubSubNode<Relay> &,
		std::string channel [[maybe_unused]]
	) {
		SPDLOG_DEBUG("Did unsubscribe: {}", channel);
	}

	void did_subscribe(
		marlin::pubsub::PubSubNode<Relay> &,
		std::string channel [[maybe_unused]]
	) {
		SPDLOG_DEBUG("Did subscribe: {}", channel);
	}

	void did_recv_message(
		marlin::pubsub::PubSubNode<Relay> &,
		std::unique_ptr<char[]> &&,
		uint64_t,
		std::string &channel [[maybe_unused]],
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

	void manage_subscribers(
		std::string,
		typename marlin::pubsub::PubSubNode<Relay>::TransportSet&,
		typename marlin::pubsub::PubSubNode<Relay>::TransportSet&) {
	}
};

#endif // MARLIN_RELAY_RELAY_HPP
