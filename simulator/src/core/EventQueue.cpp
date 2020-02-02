#include "marlin/simulator/core/EventQueue.hpp"


namespace marlin {
namespace simulator {

void EventQueue::add_event(std::shared_ptr<Event<EventQueue>> event) {
	storage.add(event);
}

void EventQueue::remove_event(std::shared_ptr<Event<EventQueue>> event) {
	storage.get<0>().remove(event);
}

void EventQueue::remove_event(uint64_t event_id) {
	storage.get<0>().remove(event_id);
}

bool EventQueue::is_empty() {
	return storage.is_empty();
}

std::shared_ptr<Event<EventQueue>> EventQueue::get_next_event() {
	return storage.get<1>().front();
}

} // namespace simulator
} // namespace marlin
