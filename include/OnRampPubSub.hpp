#include "PubSubNode.hpp"

namespace marlin {
namespace pubsub {

template<typename PubSubDelegate>
class OnRampPubSub : public PubSubNode<PubSubDelegate> {

public:
	OnRampPubSub(const net::SocketAddress &addr) : PubSubNode<PubSubDelegate>(addr) {};

protected:
	void ManageSubscribers();
};

/* TODO:
	ensure that the churn rate is not too high
	provide it as an abstract interface
	send dummy packet to estimate rtt
	Put maxSubsriptions in a config
*/
template<typename PubSubDelegate>
void OnRampPubSub<PubSubDelegate>::ManageSubscribers() {

	SPDLOG_DEBUG("Managing peers");

	std::for_each(
		this->delegate->channels.begin(),
		this->delegate->channels.end(),
		[&] (std::string const channel) {

			// move some of the subscribers to potential subscribers if oversubscribed
			if (this->channel_subscriptions[channel].size() > DefaultMaxSubscriptions) {
				// insert churn algorithm here
				// send message to removed and added peers
			}

			for (auto* pot_transport :this-> potential_channel_subscriptions[channel]) {
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