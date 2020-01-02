#include "./AsyncEvent.h"

int AsyncEvent::count = 0;

AsyncEvent::AsyncEvent(Event _event, long _tickToExecOn) {
	event = _event;
	tickToExecOn = _tickToExecOn;
	count++;
}

bool AsyncEvent::operator<(const AsyncEvent& e) const { 
	if(tickToExecOn == e.tickToExecOn) return count < e.count;
	return tickToExecOn < e.tickToExecOn; 
} 

long AsyncEvent::getTickToExecOn() const {
	return tickToExecOn;
}