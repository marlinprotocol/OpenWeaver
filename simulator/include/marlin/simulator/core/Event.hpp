#ifndef MARLIN_SIMULATOR_CORE_EVENT_HPP
#define MARLIN_SIMULATOR_CORE_EVENT_HPP

#include <cstdint>

namespace marlin {
namespace simulator {

template<typename EventManager>
class Event {
private:
	static uint64_t id_seq;
protected:
	uint64_t id;
	uint64_t tick;

public:
	Event(uint64_t tick);

	inline uint64_t get_tick() {
		return tick;
	}

	inline uint64_t get_id() {
		return id;
	}

	virtual void run(EventManager &manager) = 0;
};


// Impl

template<typename EventManager>
uint64_t Event<EventManager>::id_seq = 0;

template<typename EventManager>
Event<EventManager>::Event(uint64_t tick) {
	id = id_seq++;
	this->tick = tick;
}

} // namespace simulator
} // namespace marlin

#endif // MARLIN_SIMULATOR_CORE_EVENT_HPP
