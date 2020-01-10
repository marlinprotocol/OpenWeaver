#ifndef MARLIN_MULTICAST_DEFAULTMULTICASTCLIENT_HPP
#define MARLIN_MULTICAST_DEFAULTMULTICASTCLIENT_HPP

#include <marlin/pubsub/PubSubNode.hpp>
#include <marlin/beacon/DiscoveryClient.hpp>
#include <uv.h>


namespace marlin {
namespace multicast {

#define PUBSUB_PROTOCOL_NUMBER 0x10000000

struct DefaultMulticastClientOptions {
	uint8_t* static_sk;
	std::vector<std::string> channels = {"default"};
	std::string beacon_addr = "127.0.0.1:9002";
	std::string discovery_addr = "127.0.0.1:8002";
	std::string pubsub_addr = "127.0.0.1:8000";
	size_t max_conn = 2;
};

template<typename Delegate>
class DefaultMulticastClient {
public:
	using Self = DefaultMulticastClient<Delegate>;
	using PubSubNodeType = pubsub::PubSubNode<
		Self,
		false,
		false,
		false
	>;

	beacon::DiscoveryClient<Self> b;
	PubSubNodeType ps;

	void new_peer(
		net::SocketAddress const &addr,
		uint8_t const* static_pk,
		uint32_t protocol,
		uint16_t
	) {
		if(protocol == PUBSUB_PROTOCOL_NUMBER) {
			ps.subscribe(addr, static_pk);
		}
	}

	std::vector<std::tuple<uint32_t, uint16_t, uint16_t>> get_protocols() {
		return {};
	}

	void did_unsubscribe(
		PubSubNodeType &,
		std::string channel
	) {
		SPDLOG_DEBUG("Did unsubscribe: {}", channel);
		delegate->did_unsubscribe(*this, channel);
	}

	void did_subscribe(
		PubSubNodeType &,
		std::string channel
	) {
		SPDLOG_DEBUG("Did subscribe: {}", channel);
		delegate->did_subscribe(*this, channel);
	}

	void did_recv_message(
		PubSubNodeType &,
		net::Buffer &&message,
		net::Buffer &&witness,
		std::string &channel,
		uint64_t message_id
	) {
		SPDLOG_DEBUG(
			"Received message {} on channel {}",
			message_id,
			channel
		);
		delegate->did_recv_message(
			*this,
			std::move(message),
			std::move(witness),
			channel,
			message_id
		);
	}

	void manage_subscriptions(
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
					[&] (std::string const channel) {
						ps.send_UNSUBSCRIBE(*toReplaceTransport, channel);
					}
				);

				ps.remove_conn(sol_conns, *toReplaceTransport);
				ps.add_sol_standby_conn(*toReplaceTransport);
			}
		}

		if (sol_conns.size() < max_sol_conns) {
			auto* toReplaceWithTransport = sol_standby_conns.find_min_rtt_transport();

			if ( toReplaceWithTransport != nullptr) {
				auto* toReplaceWithTransport = sol_standby_conns.find_min_rtt_transport();

				SPDLOG_DEBUG("Moving address: {} from sol_standby_conns to sol_conns",
					toReplaceWithTransport->dst_addr.to_string()
				);

				ps.add_sol_conn(*toReplaceWithTransport);
			}
		}

		for (auto* transport [[maybe_unused]] : sol_standby_conns) {
			SPDLOG_DEBUG("STANDBY Sol : {}  rtt: {}", transport->dst_addr.to_string(), transport->get_rtt());
		}

		for (auto* transport [[maybe_unused]] : sol_conns) {
			SPDLOG_DEBUG("Sol : {}  rtt: {}", transport->dst_addr.to_string(), transport->get_rtt());
		}
	}

	Delegate *delegate = nullptr;
	std::vector<std::string> channels = {"default"};

	DefaultMulticastClient(
		DefaultMulticastClientOptions const& options = DefaultMulticastClientOptions()
	) : b(
			net::SocketAddress::from_string(options.discovery_addr),
			options.static_sk
		),
		ps(
			net::SocketAddress::from_string(options.pubsub_addr),
			options.max_conn,
			0,
			options.static_sk
		),
		channels(options.channels) {
		SPDLOG_INFO(
			"Beacon: {}, Discovery: {}, PubSub: {}",
			options.beacon_addr,
			options.discovery_addr,
			options.pubsub_addr
		);

		b.delegate = this;
		ps.delegate = this;

		b.start_discovery(net::SocketAddress::from_string(options.beacon_addr));
	}

	static int run_event_loop() {
		return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
	}
};

} // namespace multicast
} // namespace marlin

#endif // MARLIN_MULTICAST_DEFAULTMULTICASTCLIENT_HPP
