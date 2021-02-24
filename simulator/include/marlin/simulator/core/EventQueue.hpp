#ifndef MARLIN_SIMULATOR_CORE_EVENTQUEUE_HPP
#define MARLIN_SIMULATOR_CORE_EVENTQUEUE_HPP

#include "marlin/simulator/core/Event.hpp"
#include "marlin/simulator/core/IndexedStorage.hpp"

namespace marlin {
namespace simulator {

template<class EventManager>
class EventQueue {
private:
	IndexedStorage<
		Event<EventManager>,
		MapIndex<
			Event<EventManager>,
			uint64_t,
			&Event<EventManager>::get_id
		>,
		MultimapIndex<
			Event<EventManager>,
			uint64_t,
			&Event<EventManager>::get_tick
		>
	> storage;
public:
	void add_event(Event<EventManager>* event);
	void remove_event(Event<EventManager>* event);
	void remove_event(uint64_t event_id);
	Event<EventManager>* get_next_event();
	bool is_empty();
};


// Impl

template<typename EventManager>
void EventQueue<EventManager>::add_event(Event<EventManager>* event) {
	storage.add(event);
}

template<typename EventManager>
void EventQueue<EventManager>::remove_event(Event<EventManager>* event) {
	storage.remove(event);
	delete event;
}

template<typename EventManager>
void EventQueue<EventManager>::remove_event(uint64_t event_id) {
	remove(storage.template get<0>().at(event_id));
}

template<typename EventManager>
bool EventQueue<EventManager>::is_empty() {
	return storage.is_empty();
}

template<typename EventManager>
Event<EventManager>* EventQueue<EventManager>::get_next_event() {
	return storage.template get<1>().front();
}


} // namespace simulator
} // namespace marlin

#endif // MARLIN_SIMULATOR_CORE_EVENTQUEUE_HPP
