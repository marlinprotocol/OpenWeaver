#include "./Event.h"

Event::Event(uint64_t _durationInTicks) : durationInTicks(_durationInTicks) {}

uint64_t Event::getDurationInTicks() {
	return durationInTicks;
} 
