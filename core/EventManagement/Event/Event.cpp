#include "./Event.h"

Event::Event(long long _durationInTicks) : durationInTicks(_durationInTicks) {}

long Event::getDurationInTicks() {
	return durationInTicks;
} 
