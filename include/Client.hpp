#ifndef MARLIN_RELAY_CLIENT_HPP
#define MARLIN_RELAY_CLIENT_HPP

#include <marlin/pubsub/PubSubNode.hpp>
#include <marlin/beacon/DiscoveryClient.hpp>

using namespace marlin;
using namespace marlin::net;
using namespace marlin::beacon;
using namespace marlin::pubsub;

#define CLIENT_PUBSUB_PROTOCOL_NUMBER 0x10000002

class Client {
private:

	using PubSubNodeType = marlin::pubsub::PubSubNode<
		Client,
		false,
		false,
		false
	>;

	static const uint32_t my_protocol = CLIENT_PUBSUB_PROTOCOL_NUMBER;
	bool is_discoverable = true; // false for client
	PubSubNodeType *ps;
	marlin::beacon::DiscoveryClient<Client> *b;

public:

	Client(
		const net::SocketAddress &pubsub_addr,
		const net::SocketAddress &beacon_addr,
		const net::SocketAddress &beacon_server_addr
	) {
		// setting up pusbub and beacon variables
		ps = new PubSubNodeType(pubsub_addr);
		ps->delegate = this;
		b = new DiscoveryClient<Client>(beacon_addr);
		b->is_discoverable = this->is_discoverable;
		b->delegate = this;

		b->start_discovery(beacon_server_addr);
	}


	std::vector<std::tuple<uint32_t, uint16_t, uint16_t>> get_protocols() {
		return {
			std::make_tuple(my_protocol, 0, 8000)
		};
	}

	// relay logic
	void new_peer(
		net::SocketAddress const &addr,
		uint32_t protocol,
		uint16_t
	) {
		{
			if(protocol == RELAY_PUBSUB_PROTOCOL_NUMBER) {
				ps->subscribe(addr);
				ps->add_sol_conn(addr);
			}
		}
	}

	std::vector<std::string> channels = {"eth"};

	void did_unsubscribe(
		PubSubNodeType &,
		std::string channel __attribute__((unused))
	) {
		SPDLOG_DEBUG("Did unsubscribe: {}", channel);
	}

	void did_subscribe(
		PubSubNodeType &,
		std::string channel __attribute__((unused))
	) {
		SPDLOG_DEBUG("Did subscribe: {}", channel);
	}

	void did_recv_message(
		PubSubNodeType &,
		Buffer &&message __attribute__((unused)),
		std::string &channel __attribute__((unused)),
		uint64_t message_id __attribute__((unused))
	) {
		SPDLOG_INFO(
			"Received message {} on channel {}",
			message_id,
			channel
		);
	}

	void manage_subscribers(
		std::string,
		PubSubNodeType::TransportSet&,
		PubSubNodeType::TransportSet&) {
	}
};

#endif // MARLIN_RELAY_CLIENT_HPP
