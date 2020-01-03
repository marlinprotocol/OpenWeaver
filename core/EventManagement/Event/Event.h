#ifndef EVENT_H_
#define EVENT_H_

#include "./EventTypes/EventType.h"

class Event {
private:
	long durationInTicks;
	EventType eventType;

public:
	Event();
	long getDurationInTicks();
	virtual bool execute() = 0;
	virtual ~Event() {}
};

#endif /*EVENT_H_*/