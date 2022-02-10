#ifndef MARLIN_MULTICAST_DEFAULTMULTICASTCLIENT_HPP
#define MARLIN_MULTICAST_DEFAULTMULTICASTCLIENT_HPP

#include <marlin/pubsub/PubSubNode.hpp>
#include <marlin/pubsub/witness/LpfBloomWitnesser.hpp>
#include <marlin/beacon/ClusterDiscoverer.hpp>
#include <tuple>


namespace marlin {
namespace multicast {

#define PUBSUB_PROTOCOL_NUMBER 0x10000000

struct DefaultMulticastClientOptions {
	uint8_t* static_sk;
	uint8_t* static_pk;
	std::vector<uint16_t> channels = {0};
	std::string beacon_addr = "127.0.0.1:9002";
	std::string discovery_addr = "127.0.0.1:8002";
	std::string pubsub_addr = "127.0.0.1:8000";
	std::string staking_url = "/subgraphs/name/marlinprotocol/staking";
	std::string network_id = "0xaaaebeba3810b1e6b70781f14b2d72c1cb89c0b2b320c43bb67ff79f562f5ff4";
	size_t max_conn = 2;
};


struct MaskAll {
	static uint64_t mask(
		core::WeakBuffer
	) {
		return 0x0;
	}
};


template<typename Delegate, typename AttesterType = pubsub::EmptyAttester, typename WitnesserType = pubsub::LpfBloomWitnesser, typename LogMask = MaskAll>
class DefaultMulticastClient {
public:
	using Self = DefaultMulticastClient<Delegate, AttesterType, WitnesserType, LogMask>;
	using PubSubNodeType = pubsub::PubSubNode<
		Self,
		false,
		false,
		false,
		AttesterType,
		WitnesserType
	>;

	beacon::ClusterDiscoverer<Self> b;
	PubSubNodeType ps;

	void new_peer_protocol(
		std::array<uint8_t, 20> client_key,
		core::SocketAddress const &addr,
		uint8_t const* static_pk,
		uint32_t protocol,
		uint16_t
	) {
		if(protocol == PUBSUB_PROTOCOL_NUMBER) {
			// Skip zero keys
			if(client_key == std::array<uint8_t, 20>({})) {
				return;
			}
			ps.subscribe(client_key, addr, static_pk);
		}
	}

	std::vector<std::tuple<uint32_t, uint16_t, uint16_t>> get_protocols() {
		return {};
	}

	void did_unsubscribe(
		PubSubNodeType &,
		uint16_t channel
	) {
		SPDLOG_DEBUG("Did unsubscribe: {}", channel);
		delegate->did_unsubscribe(*this, channel);
	}

	void did_subscribe(
		PubSubNodeType &,
		uint16_t channel
	) {
		SPDLOG_DEBUG("Did subscribe: {}", channel);
		delegate->did_subscribe(*this, channel);
	}

	void did_recv(
		PubSubNodeType &,
		core::Buffer &&message,
		typename PubSubNodeType::MessageHeaderType header,
		uint16_t channel,
		uint64_t message_id
	) {
		SPDLOG_DEBUG(
			"Received message {} on channel {}",
			message_id,
			channel
		);
		delegate->did_recv(
			*this,
			std::move(message),
			header,
			channel,
			message_id
		);
	}

	void msg_log(
		core::SocketAddress taddr,
		typename PubSubNodeType::ClientKey baddr,
		std::array<uint8_t, 20> address,
		uint64_t message_id,
		core::WeakBuffer message
	) {
		if((message_id & LogMask::mask(message)) == 0) {
			SPDLOG_INFO(
				"Msg log: {}, cluster: 0x{:spn}, relay: {}, sender: 0x{:spn}",
				message_id, spdlog::to_hex(baddr.data(), baddr.data()+baddr.size()), taddr.to_string(),
				spdlog::to_hex(address.data(), address.data()+20)
			);
		}
	}

	void manage_subscriptions(
		typename PubSubNodeType::ClientKey baddr,
		size_t max_sol_conns,
		typename PubSubNodeType::TransportSet& sol_conns,
		typename PubSubNodeType::TransportSet& sol_standby_conns
	) {
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
						ps.send_UNSUBSCRIBE(*toReplaceTransport, channel);
					}
				);

				ps.remove_conn(sol_conns, *toReplaceTransport);
				ps.add_sol_standby_conn(baddr, *toReplaceTransport);
			}
		}

		if (sol_conns.size() < max_sol_conns) {
			auto* toReplaceWithTransport = sol_standby_conns.find_min_rtt_transport();

			if ( toReplaceWithTransport != nullptr) {

				SPDLOG_DEBUG("Moving address: {} from sol_standby_conns to sol_conns",
					toReplaceWithTransport->dst_addr.to_string()
				);

				ps.remove_conn(sol_standby_conns, *toReplaceWithTransport);
				ps.add_sol_conn(baddr, *toReplaceWithTransport);
			}
		}

		for (auto* transport [[maybe_unused]] : sol_standby_conns) {
			[[maybe_unused]] auto* static_pk = transport->get_static_pk();
			[[maybe_unused]] auto* remote_static_pk = transport->get_remote_static_pk();
			SPDLOG_DEBUG(
				"Node {:spn}: Standby conn: {:spn}: rtt: {}",
				spdlog::to_hex(static_pk, static_pk + crypto_box_PUBLICKEYBYTES),
				spdlog::to_hex(remote_static_pk, remote_static_pk + crypto_box_PUBLICKEYBYTES),
				transport->get_rtt()
			);
		}

		for (auto* transport [[maybe_unused]] : sol_conns) {
			[[maybe_unused]] auto* static_pk = transport->get_static_pk();
			[[maybe_unused]] auto* remote_static_pk = transport->get_remote_static_pk();
			SPDLOG_DEBUG(
				"Node {:spn}: Sol conn: {:spn}: rtt: {}",
				spdlog::to_hex(static_pk, static_pk + crypto_box_PUBLICKEYBYTES),
				spdlog::to_hex(remote_static_pk, remote_static_pk + crypto_box_PUBLICKEYBYTES),
				transport->get_rtt()
			);
		}
	}

	Delegate *delegate = nullptr;
	std::vector<uint16_t> channels = {0};

	template<typename... Args>
	DefaultMulticastClient(
		DefaultMulticastClientOptions const& options,
		Args&&... attester_args
	) : b(
			core::SocketAddress::from_string(options.discovery_addr),
			options.static_sk
		),
		ps(
			core::SocketAddress::from_string(options.pubsub_addr),
			options.max_conn,
			0,
			options.static_sk,
			std::forward_as_tuple(options.staking_url, options.network_id),
			std::forward_as_tuple(std::forward<Args>(attester_args)...),
			std::tie(options.static_pk)
		),
		channels(options.channels) {
		SPDLOG_DEBUG(
			"Beacon: {}, Discovery: {}, PubSub: {}",
			options.beacon_addr,
			options.discovery_addr,
			options.pubsub_addr
		);

		b.delegate = this;
		ps.delegate = this;

		b.start_discovery(core::SocketAddress::from_string(options.beacon_addr));
	}

	static int run_event_loop() {
		return asyncio::EventLoop::run();
	}
};

} // namespace multicast
} // namespace marlin

#endif // MARLIN_MULTICAST_DEFAULTMULTICASTCLIENT_HPP
