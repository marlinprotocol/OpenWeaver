#ifndef MARLIN_SIMULATOR_TIMER_TIMEREVENT_HPP
#define MARLIN_SIMULATOR_TIMER_TIMEREVENT_HPP

#include "marlin/simulator/core/Event.hpp"


namespace marlin {
namespace simulator {

template<
	typename EventManager,
	typename TargetType,
	void (TargetType::*callback)()
>
class TimerEvent : public Event<EventManager> {
private:
	TargetType& target;

public:
	TimerEvent(
		uint64_t tick,
		TargetType &target
	) : Event<EventManager>(tick), target(target) {}
	virtual ~TimerEvent() {}

	virtual void run(EventManager&) override {
		(target.*callback)();
	}
};

} // namespace simulator
} // namespace marlin

#endif // MARLIN_SIMULATOR_TIMER_TIMEREVENT_HPP
