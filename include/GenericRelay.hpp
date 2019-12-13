#ifndef MARLIN_RELAY_GENERIC_RELAY_HPP
#define MARLIN_RELAY_GENERIC_RELAY_HPP

#include <marlin/pubsub/PubSubNode.hpp>
#include <marlin/beacon/DiscoveryClient.hpp>

using namespace marlin;
using namespace marlin::net;
using namespace marlin::beacon;
using namespace marlin::pubsub;

#define MASTER_PUBSUB_PROTOCOL_NUMBER 0x10000000
#define RELAY_PUBSUB_PROTOCOL_NUMBER 0x10000001

template<
	bool enable_cut_through = false,
	bool accept_unsol_conn = false,
	bool enable_relay = false
>
class GenericRelay {
private:

	using Self = GenericRelay<
		enable_cut_through,
		accept_unsol_conn,
		enable_relay
	>;

	using PubSubNodeType = marlin::pubsub::PubSubNode<
		Self,
		enable_cut_through,
		accept_unsol_conn,
		enable_relay
	>;

	uint32_t my_protocol;
	bool is_discoverable = true; // false for client
	PubSubNodeType *ps;
	marlin::beacon::DiscoveryClient<Self> *b;

public:

	GenericRelay(
		uint32_t protocol,
		const net::SocketAddress &pubsub_addr,
		const net::SocketAddress &beacon_addr,
		const net::SocketAddress &beacon_server_addr
	) {
		// setting protocol
		my_protocol = protocol;

		// setting up pusbub and beacon
		if (
			protocol == MASTER_PUBSUB_PROTOCOL_NUMBER ||
			protocol == RELAY_PUBSUB_PROTOCOL_NUMBER
		) {
			ps = new PubSubNodeType(pubsub_addr);
			ps->delegate = this;
			b = new DiscoveryClient<Self>(beacon_addr);
			b->is_discoverable = this->is_discoverable;
			b->delegate = this;

			b->start_discovery(beacon_server_addr);
		}
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
		if (
			my_protocol==MASTER_PUBSUB_PROTOCOL_NUMBER ||
			my_protocol==RELAY_PUBSUB_PROTOCOL_NUMBER
		) {
			if(protocol == MASTER_PUBSUB_PROTOCOL_NUMBER) {
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
		typename PubSubNodeType::TransportSet&,
		typename PubSubNodeType::TransportSet&) {
	}
};

#endif // MARLIN_RELAY_GENERIC_RELAY_HPP
