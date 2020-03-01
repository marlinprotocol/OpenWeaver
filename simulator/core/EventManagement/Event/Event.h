#ifndef EVENT_H_
#define EVENT_H_

#include "./EventTypes/EventType.h"
#include "../../Network/Network.h"
#include "../../../helpers/Logger/easylogging.h"

class EventQueue;

class Event {
private:
	uint64_t durationInTicks;

public:
	Event(uint64_t _durationInTicks);
	uint64_t getDurationInTicks();
	virtual bool execute(Network& _network, EventManager* _eventManager, uint64_t _currentTick) = 0;
	virtual ~Event() {}
};

#endif /*EVENT_H_*/