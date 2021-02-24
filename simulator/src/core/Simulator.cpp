#include "marlin/simulator/core/Simulator.hpp"


namespace marlin {
namespace simulator {

Simulator Simulator::default_instance = Simulator();

Simulator::Simulator() {}

void Simulator::add_event(Event<Simulator>* event) {
	if(!queue.is_empty() && event->get_tick() < queue.get_next_event()->get_tick()) {
		// Simulations cannot move backward
		throw;
	}
	queue.add_event(event);
}

void Simulator::remove_event(Event<Simulator>* event) {
	queue.remove_event(event);
}

void Simulator::run() {
	while(!queue.is_empty()) {
		auto event = queue.get_next_event();
		event->run(*this);
		queue.remove_event(event);
	}
}

uint64_t Simulator::current_tick() {
	if(queue.is_empty()) {
		return 0;
	} else {
		auto event = queue.get_next_event();
		return event->get_tick();
	}
}

} // namespace simulator
} // namespace marlin
