#ifndef MARLIN_SIMULATOR_CORE_SIMULATOR_HPP
#define MARLIN_SIMULATOR_CORE_SIMULATOR_HPP

#include "marlin/simulator/core/EventQueue.hpp"


namespace marlin {
namespace simulator {

class Simulator {
public:
	EventQueue queue;
	Simulator();

	void add_event(std::shared_ptr<Event<EventQueue>> event);
	void run();
};

} // namespace simulator
} // namespace marlin

#endif // MARLIN_SIMULATOR_CORE_SIMULATOR_HPP
