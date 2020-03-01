#ifndef MARLIN_SIMULATOR_NETWORK_DATAONLINKEVENT_HPP
#define MARLIN_SIMULATOR_NETWORK_DATAONLINKEVENT_HPP

#include "marlin/simulator/core/Event.hpp"

#include <functional>


namespace marlin {
namespace simulator {

template<typename EventManager, typename MessageType>
class DataOnLinkEvent : public Event<EventManager> {
private:
	MessageType message;
	std::function<void(MessageType &&, uint64_t, EventManager&)> f_did_recv;
public:
	DataOnLinkEvent(
		uint64_t tick,
		MessageType &&message,
		std::function<void(MessageType &&, uint64_t, EventManager&)> f_did_recv
	) : Event<EventManager>(tick), message(std::move(message)), f_did_recv(f_did_recv) {}
	virtual ~DataOnLinkEvent() {}

	void run(EventManager& manager) override {
		f_did_recv(std::move(message), this->tick, manager);
	}
};

} // namespace simulator
} // namespace marlin

#endif // MARLIN_SIMULATOR_NETWORK_DATAONLINKEVENT_HPP
