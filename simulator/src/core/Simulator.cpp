#include "marlin/simulator/core/Simulator.hpp"


namespace marlin {
namespace simulator {

Simulator::Simulator() {}

void Simulator::add_event(std::shared_ptr<Event<EventQueue>> event) {
	queue.add_event(event);
}

void Simulator::run() {
	while(!queue.is_empty()) {
		auto event = queue.get_next_event();
		event->run(queue);
		queue.remove_event(event);
	}
}

} // namespace simulator
} // namespace marlin
