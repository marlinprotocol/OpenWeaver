#include "PubSubNode.hpp"

namespace marlin {
namespace pubsub {

template<typename PubSubDelegate>
class OnRampPubSub : public PubSubNode<PubSubDelegate> {

public:
	OnRampPubSub(const net::SocketAddress &addr) : PubSubNode<PubSubDelegate>(addr) {};

protected:
	void manage_subscribers();
};

/* TODO:
	ensure that the churn rate is not too high
	provide it as an abstract interface
	send dummy packet to estimate rtt
	Put maxSubsriptions in a config
*/
template<typename PubSubDelegate>
void OnRampPubSub<PubSubDelegate>::manage_subscribers() {

	SPDLOG_INFO("Managing peers");

	std::for_each(
		this->delegate->channels.begin(),
		this->delegate->channels.end(),
		[&] (std::string const channel) {

			typename PubSubNode<PubSubDelegate>::TransportSet& temp_transport_set = this->channel_subscriptions[channel];
			typename PubSubNode<PubSubDelegate>::TransportSet& temp_potential_transport_set = this->potential_channel_subscriptions[channel];

			// move some of the subscribers to potential subscribers if oversubscribed
			if (temp_transport_set.size() >= DefaultMaxSubscriptions) {
				// insert churn algorithm here. need to find a better algorithm to give old bad performers a chance gain. Pick randomly from potential peers?
				// send message to removed and added peers

				typename PubSubNode<PubSubDelegate>::BaseTransport* toReplaceTransport = this->find_max_rtt_transport(temp_transport_set);
				typename PubSubNode<PubSubDelegate>::BaseTransport* toReplaceWithTransport = this->find_random_rtt_transport(temp_potential_transport_set);

				if (toReplaceTransport != nullptr &&
					toReplaceWithTransport != nullptr) {

					SPDLOG_INFO("Moving address: {} from potential subscribers to subscribers list on channel: {} ",
						toReplaceWithTransport->dst_addr.to_string(),
						channel);

					temp_potential_transport_set.erase(toReplaceWithTransport);
					temp_transport_set.insert(toReplaceWithTransport);

					PubSubNode<PubSubDelegate>::send_RESPONSE(*toReplaceWithTransport, true, "SUBSCRIBED TO " + channel);

					SPDLOG_INFO("Moving address: {} from subscribers to potential subscribers list on channel: {} ",
						toReplaceTransport->dst_addr.to_string(),
						channel);

					temp_transport_set.erase(toReplaceTransport);
					temp_potential_transport_set.insert(toReplaceTransport);

					PubSubNode<PubSubDelegate>::send_RESPONSE(*toReplaceTransport, true, "UNSUBSCRIBED FROM " + channel);
				}
			}

			for (auto* pot_transport : temp_potential_transport_set) {
				SPDLOG_INFO("Potential Subscriber: {}  rtt: {} on channel {}", pot_transport->dst_addr.to_string(), pot_transport->get_rtt(), channel);
			}

			for (auto* transport : temp_transport_set) {
				SPDLOG_INFO("Subscriber: {}  rtt: {} on channel {}", transport->dst_addr.to_string(),  transport->get_rtt(), channel);
			}
		}
	);
} 


} // namespace pubsub
} // namespace marlin