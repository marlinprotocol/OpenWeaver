#ifndef MARLIN_SIMULATOR_CORE_EVENTQUEUE_HPP
#define MARLIN_SIMULATOR_CORE_EVENTQUEUE_HPP

#include "marlin/simulator/core/Event.hpp"
#include "marlin/simulator/core/IndexedStorage.hpp"

namespace marlin {
namespace simulator {

class EventQueue {
private:
	IndexedStorage<
		Event<EventQueue>,
		MapIndex<
			Event<EventQueue>,
			uint64_t,
			&Event<EventQueue>::get_id
		>,
		MultimapIndex<
			Event<EventQueue>,
			uint64_t,
			&Event<EventQueue>::get_tick
		>
	> storage;
public:
	void add_event(std::shared_ptr<Event<EventQueue>> event);
	void remove_event(std::shared_ptr<Event<EventQueue>> event);
	void remove_event(uint64_t event_id);
	std::shared_ptr<Event<EventQueue>> get_next_event();
};

} // namespace simulator
} // namespace marlin

#endif // MARLIN_SIMULATOR_CORE_EVENTQUEUE_HPP
