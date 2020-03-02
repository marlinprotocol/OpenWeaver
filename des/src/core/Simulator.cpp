#include "marlin/simulator/core/Simulator.hpp"


namespace marlin {
namespace simulator {

Simulator::Simulator() {}

void Simulator::add_event(std::shared_ptr<Event<Simulator>> event) {
	if(!queue.is_empty() && event->get_tick() < queue.get_next_event()->get_tick()) {
		// Simulations cannot move backward
		throw;
	}
	queue.add_event(event);
}

void Simulator::run() {
	while(!queue.is_empty()) {
		auto event = queue.get_next_event();
		event->run(*this);
		queue.remove_event(event);
	}
}

} // namespace simulator
} // namespace marlin
