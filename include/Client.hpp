#ifndef MARLIN_RELAY_CLIENT_HPP
#define MARLIN_RELAY_CLIENT_HPP

#include <marlin/pubsub/PubSubNode.hpp>
#include <marlin/beacon/DiscoveryClient.hpp>
#include <fstream>
#include <experimental/filesystem>

using namespace marlin;
using namespace marlin::net;
using namespace marlin::beacon;
using namespace marlin::pubsub;

#define RELAY_PUBSUB_PROTOCOL_NUMBER 0x10000001
#define CLIENT_PUBSUB_PROTOCOL_NUMBER 0x10000002

class Client {
private:
	size_t max_sol_conns = 2;
	uint32_t pubsub_port;

	using PubSubNodeType = marlin::pubsub::PubSubNode<
		Client,
		false,
		false,
		false
	>;

	// const uint32_t my_protocol = CLIENT_PUBSUB_PROTOCOL_NUMBER;
	bool is_discoverable = true; // false for client
	PubSubNodeType *ps;
	marlin::beacon::DiscoveryClient<Client> *b;

	uint8_t static_sk[crypto_box_SECRETKEYBYTES];
	uint8_t static_pk[crypto_box_PUBLICKEYBYTES];

public:

	Client(
		uint32_t pubsub_port,
		const net::SocketAddress &pubsub_addr,
		const net::SocketAddress &beacon_addr,
		const net::SocketAddress &beacon_server_addr
	) {
		//PROTOCOL HACK
		this->pubsub_port = pubsub_port;

		//setting up keys

		if(std::experimental::filesystem::exists("./.marlin/keys/static")) {
			std::ifstream sk("./.marlin/keys/static", std::ios::binary);
			if(!sk.read((char *)static_sk, crypto_box_SECRETKEYBYTES)) {
				throw;
			}
			sk.close();
			crypto_scalarmult_base(static_pk, static_sk);
		} else {
			crypto_box_keypair(static_pk, static_sk);

			std::experimental::filesystem::create_directories("./.marlin/keys/");
			std::ofstream sk("./.marlin/keys/static", std::ios::binary);

			sk.write((char *)static_sk, crypto_box_SECRETKEYBYTES);
			sk.close();
		}

		SPDLOG_DEBUG(
			"PK: {:spn}, SK: {:spn}",
			spdlog::to_hex(static_pk, static_pk+32),
			spdlog::to_hex(static_sk, static_sk+32)
		);

		// setting up pusbub and beacon variables
		ps = new PubSubNodeType(pubsub_addr, max_sol_conns, 0, static_sk);
		ps->delegate = this;
		b = new DiscoveryClient<Client>(beacon_addr, static_sk);
		b->is_discoverable = this->is_discoverable;
		b->delegate = this;

		b->start_discovery(beacon_server_addr);
	}


	std::vector<std::tuple<uint32_t, uint16_t, uint16_t>> get_protocols() {
		return {
			// std::make_tuple(my_protocol, 0, pubsub_port)
		};
	}

	void new_peer(
		net::SocketAddress const &addr,
		uint8_t const* static_pk,
		uint32_t protocol,
		uint16_t version
	) {
		SPDLOG_INFO(
			"New peer: {}, {:spn}, {}, {}",
			addr.to_string(),
			spdlog::to_hex(static_pk, static_pk+32),
			protocol,
			version
		);

		if(protocol == RELAY_PUBSUB_PROTOCOL_NUMBER) {
			ps->subscribe(addr, static_pk);
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

	void manage_subscriptions(
		size_t max_sol_conns,
		typename PubSubNodeType::TransportSet& sol_conns,
		typename PubSubNodeType::TransportSet& sol_standby_conns
	) {
		// TODO: remove comment
		SPDLOG_INFO(
			"manage_subscriptions port: {} sol_conns size: {}",
				pubsub_port,
				sol_conns.size()
		);

		// move some of the subscribers to potential subscribers if oversubscribed
		// insert churn algorithm here. need to find a better algorithm to give old bad performers a chance gain. Pick randomly from potential peers?
		if (sol_conns.size() >= max_sol_conns) {
			auto* toReplaceTransport = sol_conns.find_max_rtt_transport();

			if (toReplaceTransport != nullptr) {

				SPDLOG_INFO("Moving address: {} from sol_conns to sol_standby_conns",
					toReplaceTransport->dst_addr.to_string()
				);

				// TODO: do away with individual subscribe for each channel
				std::for_each(
					channels.begin(),
					channels.end(),
					[&] (std::string const channel) {
						ps->send_UNSUBSCRIBE(*toReplaceTransport, channel);
					}
				);

				ps->remove_conn(sol_conns, *toReplaceTransport);
				ps->add_sol_standby_conn(*toReplaceTransport);
			}
		}

		if (sol_conns.size() < max_sol_conns) {
			auto* toReplaceWithTransport = sol_standby_conns.find_min_rtt_transport();

			if ( toReplaceWithTransport != nullptr) {
				auto* toReplaceWithTransport = sol_standby_conns.find_min_rtt_transport();


				SPDLOG_INFO("Moving address: {} from sol_standby_conns to sol_conns",
					toReplaceWithTransport->dst_addr.to_string()
				);

				ps->add_sol_conn(*toReplaceWithTransport);
			}
		}


		for (auto* transport : sol_standby_conns) {
			SPDLOG_INFO("STANDBY Sol : {}  rtt: {}", transport->dst_addr.to_string(), transport->get_rtt());
		}

		for (auto* transport : sol_conns) {
			SPDLOG_INFO("Sol : {}  rtt: {}", transport->dst_addr.to_string(), transport->get_rtt());
		}
	}
};

#endif // MARLIN_RELAY_CLIENT_HPP
