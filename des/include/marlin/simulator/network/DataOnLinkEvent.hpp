#ifndef MARLIN_SIMULATOR_NETWORK_DATAONLINKEVENT_HPP
#define MARLIN_SIMULATOR_NETWORK_DATAONLINKEVENT_HPP

#include "marlin/simulator/core/Event.hpp"

#include <functional>


namespace marlin {
namespace simulator {

template<
	typename EventManager,
	typename MessageType,
	typename TargetType,
	typename ...Args
>
class DataOnLinkEvent : public Event<EventManager> {
private:
	MessageType message;
	TargetType &target;
	std::tuple<Args...> args;

protected:
	template<size_t ...I>
	void run_impl(EventManager& manager, std::index_sequence<I...>) {
		target.did_recv(
			*this,
			manager,
			std::move(message),
			std::get<I>(args)...
		);
	}

public:
	DataOnLinkEvent(
		uint64_t tick,
		MessageType &&message,
		TargetType &target,
		Args&&... args
	) : Event<EventManager>(tick), message(std::move(message)), target(target), args(std::forward<Args>(args)...) {}
	virtual ~DataOnLinkEvent() {}

	virtual void run(EventManager& manager) override {
		run_impl(manager, std::index_sequence_for<Args...>{});
	}
};

} // namespace simulator
} // namespace marlin

#endif // MARLIN_SIMULATOR_NETWORK_DATAONLINKEVENT_HPP
