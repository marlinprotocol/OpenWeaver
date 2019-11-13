#ifndef MARLIN_PUBSUB_PUBSUBTRANSPORTSET_HPP
#define MARLIN_PUBSUB_PUBSUBTRANSPORTSET_HPP

namespace marlin {
namespace pubsub {

template<typename BaseTransport>
class PubSubTransportSet : public std::unordered_set<BaseTransport*> {
public:
	BaseTransport* find_random_rtt_transport();
	BaseTransport* find_min_rtt_transport();
	BaseTransport* find_max_rtt_transport();
	bool check_tranport_in_set(BaseTransport&);
};


// Impl

template<typename BaseTransport>
BaseTransport*
PubSubTransportSet<BaseTransport>::find_random_rtt_transport() {
	int set_size = this->size();

	if (set_size != 0) {
		int random_to_return = rand() % set_size;
		return *(std::next(this->begin(), random_to_return));
	}

	return nullptr;
}

// Pick the ones with rtt's -1 too to give them a chance
template<typename BaseTransport>
BaseTransport*
PubSubTransportSet<BaseTransport>::find_min_rtt_transport() {
	BaseTransport* to_return = nullptr;
	for (auto* temp_transport : *this) {
		if (temp_transport->get_rtt() == -1)
			return temp_transport;
		if (to_return == nullptr || temp_transport->get_rtt() < to_return->get_rtt()) {
			to_return = temp_transport;
		}
	}

	return to_return;
}

template<typename BaseTransport>
BaseTransport*
PubSubTransportSet<BaseTransport>::find_max_rtt_transport() {
	BaseTransport* to_return = nullptr;
	for (auto* temp_transport : *this) {
		if (temp_transport->get_rtt() == -1) continue;
		if (to_return == nullptr || temp_transport->get_rtt() > to_return->get_rtt()) {
			to_return = temp_transport;
		}
	}

	return to_return;
}

template<typename BaseTransport>
bool
PubSubTransportSet<BaseTransport>::check_tranport_in_set(
	BaseTransport& base_transport
) {
	if (this->find(&base_transport) == this->end()) {
		return false;
	}

	return true;
}

} // namespace pubsub
} // namespace marlin

#endif // MARLIN_PUBSUB_PUBSUBTRANSPORTSET_HPP
