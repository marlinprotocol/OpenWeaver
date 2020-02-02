#include "marlin/simulator/core/Event.hpp"


namespace marlin {
namespace simulator {

template<typename EventManager>
uint64_t Event<EventManager>::id_seq = 0;

template<typename EventManager>
Event<EventManager>::Event(uint64_t tick) {
	id = id_seq++;
	this->tick = tick;
}

} // namespace simulator
} // namespace marlin
