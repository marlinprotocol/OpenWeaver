#ifndef MARLIN_RELAY_RELAY_HPP
#define MARLIN_RELAY_RELAY_HPP

#include <marlin/pubsub/PubSubNode.hpp>
#include <marlin/pubsub/witness/BloomWitnesser.hpp>
#include <marlin/beacon/DiscoveryClient.hpp>

#include <boost/filesystem.hpp>

using namespace marlin;
using namespace marlin::core;
using namespace marlin::asyncio;
using namespace marlin::stream;
using namespace marlin::beacon;
using namespace marlin::pubsub;

#define MASTER_PUBSUB_PROTOCOL_NUMBER 0x10000000
#define RELAY_PUBSUB_PROTOCOL_NUMBER 0x10000000

template<
	bool enable_cut_through = false,
	bool accept_unsol_conn = false,
	bool enable_relay = false,
	template<typename, typename...> class AbciTemplate = DefaultAbci
>
class Relay {
private:
	using Self = Relay<
		enable_cut_through,
		accept_unsol_conn,
		enable_relay,
		AbciTemplate
	>;

	using PubSubNodeType = PubSubNode<
		Self,
		enable_cut_through,
		accept_unsol_conn,
		enable_relay,
		EmptyAttester,
		BloomWitnesser,
		AbciTemplate
	>;

	uint32_t protocol;
	uint32_t pubsub_port;
	PubSubNodeType *ps;
	marlin::beacon::DiscoveryClient<Self> *b;

	uint8_t static_sk[crypto_box_SECRETKEYBYTES];
	uint8_t static_pk[crypto_box_PUBLICKEYBYTES];
public:

	template<typename... Args>
	Relay(
		uint32_t protocol,
		const uint32_t pubsub_port,
		const core::SocketAddress &pubsub_addr,
		const core::SocketAddress &beacon_addr,
		const core::SocketAddress &beacon_server_addr,
		Args&&... args
	) : Relay(protocol, pubsub_port, pubsub_addr, beacon_addr, {beacon_server_addr}, {beacon_server_addr}, "", "", std::nullopt, std::forward<Args>(args)...) {}

	template<typename... Args>
	Relay(
		uint32_t protocol,
		const uint32_t pubsub_port,
		const core::SocketAddress &pubsub_addr,
		const core::SocketAddress &beacon_addr,
		std::vector<core::SocketAddress>&& discovery_addrs,
		std::vector<core::SocketAddress>&& heartbeat_addrs,
		std::string address,
		std::string name,
		std::optional<std::string> keyname,
		Args&&... args
	) {
		this->protocol = protocol;
		this->pubsub_port = pubsub_port;

		std::string keypath = "./.marlin/keys/" + keyname.value_or("static");

		if(boost::filesystem::exists(keypath)) {
			std::ifstream sk(keypath, std::ios::binary);
			if(!sk.read((char *)static_sk, crypto_box_SECRETKEYBYTES)) {
				throw;
			}
			crypto_scalarmult_base(static_pk, static_sk);
		} else {
			crypto_box_keypair(static_pk, static_sk);

			boost::filesystem::create_directories("./.marlin/keys/");
			std::ofstream sk(keypath, std::ios::binary);

			sk.write((char *)static_sk, crypto_box_SECRETKEYBYTES);
		}

		SPDLOG_DEBUG(
			"PK: {:spn}, SK: {:spn}",
			spdlog::to_hex(static_pk, static_pk+32),
			spdlog::to_hex(static_sk, static_sk+32)
		);

		auto [max_sol_conns, max_unsol_conns] = protocol == MASTER_PUBSUB_PROTOCOL_NUMBER ? std::tuple(50, 30) : std::tuple(2, 16);

		ps = new PubSubNodeType(pubsub_addr, max_sol_conns, max_unsol_conns, static_sk, {}, std::tie(static_pk), std::forward_as_tuple<Args...>(args...));
		ps->delegate = this;
		b = new DiscoveryClient<Self>(beacon_addr, static_sk);
		b->address = address;
		b->name = name;
		b->is_discoverable = true;
		b->delegate = this;

		b->start_discovery(std::move(discovery_addrs), std::move(heartbeat_addrs));
	}

	std::vector<std::tuple<uint32_t, uint16_t, uint16_t>> get_protocols() {
		return {
			std::make_tuple(this->protocol, 0, pubsub_port)
		};
	}

	// relay logic
	void new_peer_protocol(
		core::SocketAddress const &addr,
		uint8_t const* static_pk,
		uint32_t protocol,
		uint16_t version [[maybe_unused]]
	) {
		SPDLOG_DEBUG(
			"New peer: {}, {:spn}, {}, {}",
			addr.to_string(),
			spdlog::to_hex(static_pk, static_pk+32),
			protocol,
			version
		);

		if(protocol == MASTER_PUBSUB_PROTOCOL_NUMBER) {
			ps->subscribe(addr, static_pk);
		}
	}

	std::vector<uint16_t> channels = {0, 1};

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
		if(channel == 0 && (message_id & 0x0) == 0) {
			SPDLOG_INFO(
				"Received message {} on channel {}",
				message_id,
				channel
			);
		} else if(channel == 1 && (message_id & 0xf) == 0) {
			SPDLOG_INFO(
				"Received message {} on channel {}",
				message_id,
				channel
			);
		}
	}

	void manage_subscriptions(
		typename PubSubNodeType::ClientKey baddr,
		size_t max_sol_conns,
		typename PubSubNodeType::TransportSet& sol_conns,
		typename PubSubNodeType::TransportSet& sol_standby_conns
	) {
		SPDLOG_DEBUG(
			"manage_subscriptions port: {} sol_conns size: {}",
				pubsub_port,
				sol_conns.size()
		);

		// move some of the subscribers to potential subscribers if oversubscribed
		// insert churn algorithm here. need to find a better algorithm to give old bad performers a chance gain. Pick randomly from potential peers?
		if (sol_conns.size() >= max_sol_conns) {
			auto* toReplaceTransport = sol_conns.find_max_rtt_transport();

			if (toReplaceTransport != nullptr) {

				SPDLOG_DEBUG("Moving address: {} from sol_conns to sol_standby_conns",
					toReplaceTransport->dst_addr.to_string()
				);

				// TODO: do away with individual subscribe for each channel
				std::for_each(
					channels.begin(),
					channels.end(),
					[&] (uint16_t channel) {
						ps->send_UNSUBSCRIBE(*toReplaceTransport, channel);
					}
				);

				ps->remove_conn(sol_conns, *toReplaceTransport);
				ps->add_sol_standby_conn(baddr, *toReplaceTransport);
			}
		}

		if (sol_conns.size() < max_sol_conns) {
			auto* toReplaceWithTransport = sol_standby_conns.find_min_rtt_transport();

			if ( toReplaceWithTransport != nullptr) {
				auto* toReplaceWithTransport = sol_standby_conns.find_min_rtt_transport();


				SPDLOG_DEBUG("Moving address: {} from sol_standby_conns to sol_conns",
					toReplaceWithTransport->dst_addr.to_string()
				);

				ps->add_sol_conn(baddr, *toReplaceWithTransport);
			}
		}


		for (auto* transport : sol_standby_conns) {
			[[maybe_unused]] auto* remote_static_pk = transport->get_remote_static_pk();
			SPDLOG_DEBUG(
				"Node {:spn}: Standby conn: {:spn}: rtt: {}",
				spdlog::to_hex(static_pk, static_pk + crypto_box_PUBLICKEYBYTES),
				spdlog::to_hex(remote_static_pk, remote_static_pk + crypto_box_PUBLICKEYBYTES),
				transport->get_rtt()
			);
		}

		for (auto* transport : sol_conns) {
			[[maybe_unused]] auto* remote_static_pk = transport->get_remote_static_pk();
			SPDLOG_DEBUG(
				"Node {:spn}: Sol conn: {:spn}: rtt: {}",
				spdlog::to_hex(static_pk, static_pk + crypto_box_PUBLICKEYBYTES),
				spdlog::to_hex(remote_static_pk, remote_static_pk + crypto_box_PUBLICKEYBYTES),
				transport->get_rtt()
			);
		}
	}
};

#endif // MARLIN_RELAY_RELAY_HPP
