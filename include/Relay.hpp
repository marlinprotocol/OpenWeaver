#ifndef MARLIN_RELAY_RELAY_HPP
#define MARLIN_RELAY_RELAY_HPP

#include <marlin/pubsub/PubSubNode.hpp>
#include <marlin/beacon/Beacon.hpp>

using namespace marlin;
using namespace marlin::net;
using namespace marlin::beacon;
using namespace marlin::pubsub;

class Relay {
public:
	marlin::beacon::Beacon<Relay> *b;
	marlin::pubsub::PubSubNode<Relay> *ps;

	void handle_new_peer(const net::SocketAddress &addr) {
		net::SocketAddress ps_addr(addr);

		// Set correct port.
		// TODO: Improve discovery to avoid hardcoding
		reinterpret_cast<sockaddr_in *>(&ps_addr)->sin_port = htons(8000);

		ps->subscribe(ps_addr);
	}

	std::vector<std::string> channels = {"eth"};

	void did_unsubscribe(
		marlin::pubsub::PubSubNode<Relay> &,
		std::string channel
	) {
		SPDLOG_INFO("Did unsubscribe: {}", channel);
	}

	void did_subscribe(
		marlin::pubsub::PubSubNode<Relay> &,
		std::string channel
	) {
		SPDLOG_INFO("Did subscribe: {}", channel);
	}

	void did_recv_message(
		marlin::pubsub::PubSubNode<Relay> &,
		std::unique_ptr<char[]> &&message,
		uint64_t size,
		std::string &channel,
		uint64_t message_id
	) {
		SPDLOG_INFO(
			"Received message {} on channel {}: {}",
			message_id,
			channel,
			spdlog::to_hex(message.get(), message.get() + size)
		);
	}

	bool should_accept(SocketAddress const &) {
		return true;
	}
};

#endif // MARLIN_RELAY_RELAY_HPP
