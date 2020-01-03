#include "./AsyncEvent.h"

int AsyncEvent::count = 0;

AsyncEvent::AsyncEvent(std::shared_ptr<Event> _event, long _tickToExecOn) {
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

bool AsyncEvent::execute(Network& _network) {
	LOG(DEBUG) << "[AsyncEvent::execute]";
	return event->execute(_network);
}