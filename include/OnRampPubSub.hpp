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

	SPDLOG_DEBUG("Managing peers");

	std::for_each(
		this->delegate->channels.begin(),
		this->delegate->channels.end(),
		[&] (std::string const channel) {

			typename PubSubNode<PubSubDelegate>::TransportSet& temp_transport_set = this->channel_subscriptions[channel];
			typename PubSubNode<PubSubDelegate>::TransportSet& temp_potential_transport_set = this->channel_subscriptions[channel];

			typename PubSubNode<PubSubDelegate>::BaseTransport* ToReplaceTransport;
			typename PubSubNode<PubSubDelegate>::BaseTransport* ToReplaceWithTransport;

			// move some of the subscribers to potential subscribers if oversubscribed
			if (temp_transport_set.size() > DefaultMaxSubscriptions) {
				// insert churn algorithm here
				// send message to removed and added peers

				ToReplaceTransport = this->find_min_rtt_transport(temp_transport_set);
				ToReplaceWithTransport = this->find_max_rtt_transport(temp_potential_transport_set);

				if (ToReplaceTransport != NULL &&
					ToReplaceWithTransport != NULL &&
					ToReplaceTransport->get_rtt() > ToReplaceWithTransport->get_rtt()) {

					temp_transport_set.erase(ToReplaceTransport);
					temp_transport_set.insert(ToReplaceWithTransport);
					temp_potential_transport_set.erase(ToReplaceWithTransport);
					temp_potential_transport_set.insert(ToReplaceTransport);
				}
			}

			for (auto* pot_transport : temp_potential_transport_set) {
				// add condition to check if rtt is too old, ideally this should be job transport manager?
				// send dummy packet to estimate new rtt
				SPDLOG_DEBUG("Channel: {} rtt: {}", channel, pot_transport->get_rtt());
				if (pot_transport->get_rtt() == -1) {
					char *message = new char[3] {'R','T','T'};
					net::Buffer m(message, 3);

					pot_transport->send(std::move(m));
				}
			}
		}
	);
} 


} // namespace pubsub
} // namespace marlin